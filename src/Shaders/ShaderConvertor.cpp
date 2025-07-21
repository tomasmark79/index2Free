#include "ShaderConvertor.hpp"
#include <regex>
#include <sstream>
#include <algorithm>
#include <set>

ShaderConvertor::ShaderConvertor() {
    initializeFunctionReplacements();
}

ShaderConvertor::~ShaderConvertor() {
}

ShaderConversionResult ShaderConvertor::convertFromShaderToy(const std::string& shaderToyCode,
                                                              ShaderTarget target) {
    ShaderConversionResult result;

    try {
        // 1. Analýza kódu před konverzí
        auto analysis = analyzeShaderCode(shaderToyCode);
        
        // 2. Získání vertex shaderu
        result.vertexShader = getVertexShader(target);

        // 3. Začátek s hlavičkou fragment shaderu
        std::string fragmentCode = convertShaderHeader(target);

        // 4. Přidání uniformů (včetně detekovaných texture kanálů)
        fragmentCode += convertUniforms(shaderToyCode, target, analysis);

        // 5. Konverze ShaderToy built-ins
        std::string processedCode = convertShaderToyBuiltins(shaderToyCode, target);

        // 6. Konverze mainImage funkce
        processedCode = convertMainFunction(processedCode, target);

        // 7. Oprava kompatibility funkcí
        processedCode = fixCompatibilityIssues(processedCode, target);

        // 8. Finální úpravy podle analýzy
        processedCode = applyAnalysisBasedFixes(processedCode, analysis, target);

        fragmentCode += processedCode;

        result.fragmentShader = fragmentCode;
        result.success = true;
        result.analysis = analysis;

    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = "Conversion failed: " + std::string(e.what());
    }

    return result;
}

std::string ShaderConvertor::getVertexShader(ShaderTarget target) {
    switch (target) {
    case ShaderTarget::WebGL1:
        return R"(precision highp float;
attribute vec2 position;
varying vec2 vFragCoord;
void main() {
    vFragCoord = (position + 1.0) * 0.5;
    gl_Position = vec4(position, 0.0, 1.0);
})";

    case ShaderTarget::WebGL2:
        return R"(#version 300 es
precision highp float;
layout (location = 0) in vec2 position;
out vec2 vFragCoord;
void main() {
    vFragCoord = (position + 1.0) * 0.5;
    gl_Position = vec4(position, 0.0, 1.0);
})";

    case ShaderTarget::Desktop330:
    case ShaderTarget::Desktop420:
    default:
        return R"(#version 330 core
layout (location = 0) in vec2 position;
out vec2 vFragCoord;
void main() {
    vFragCoord = (position + 1.0f) * 0.5f;
    gl_Position = vec4(position, 0.0f, 1.0f);
})";
    }
}

std::string ShaderConvertor::convertShaderHeader(ShaderTarget target) {
    switch (target) {
    case ShaderTarget::WebGL1:
        return ""; // Žádná hlavička pro WebGL1

    case ShaderTarget::WebGL2:
        return "#version 300 es\n";

    case ShaderTarget::Desktop330:
        return "#version 330 core\n";

    case ShaderTarget::Desktop420:
        return "#version 420 core\n";

    default:
        return "#version 330 core\n";
    }
}

std::string ShaderConvertor::convertUniforms(const std::string& code, 
                                             ShaderTarget target, 
                                             const ShaderAnalysis& analysis) {
    std::string uniforms;

    // Pro WebGL/OpenGL ES musí být precision specifier na ZAČÁTKU
    if (target == ShaderTarget::WebGL1 || target == ShaderTarget::WebGL2) {
        uniforms += "precision mediump float;\n";
        uniforms += "precision mediump int;\n\n";
    }

    // Standardní ShaderToy uniformy
    uniforms += "uniform float iTime;\n";
    uniforms += "uniform float iTimeDelta;\n";
    uniforms += "uniform vec3 iResolution;\n";

    // Volitelné uniformy podle analýzy - hledáme jako proměnné, ne funkce
    if (code.find("iMouse") != std::string::npos) {
        uniforms += "uniform vec4 iMouse;\n";
    }
    if (code.find("iFrame") != std::string::npos) {
        uniforms += "uniform int iFrame;\n";
    }
    if (code.find("iDate") != std::string::npos) {
        uniforms += "uniform vec4 iDate;\n";
    }

    // Texture kanály podle analýzy
    for (size_t i = 0; i < analysis.textureChannels.size(); ++i) {
        uniforms += "uniform sampler2D iChannel" + std::to_string(i) + ";\n";
        if (target == ShaderTarget::WebGL1 || target == ShaderTarget::WebGL2) {
            uniforms += "uniform vec3 iChannelResolution[" + std::to_string(i + 1) + "];\n";
        }
    }

    // Input/Output proměnné podle verze
    switch (target) {
    case ShaderTarget::WebGL1:
        // WebGL1 používá varying místo in
        uniforms += "varying vec2 vFragCoord;\n\n";
        break;

    case ShaderTarget::WebGL2:
    case ShaderTarget::Desktop330:
    case ShaderTarget::Desktop420:
        uniforms += "in vec2 vFragCoord;\n";
        uniforms += "out vec4 fragColor;\n\n";
        break;
    }

    return uniforms;
}

std::string ShaderConvertor::convertMainFunction(const std::string& code, ShaderTarget target) {
    std::string result = code;

    // Pokročilejší regex pro mainImage funkci
    std::regex mainImageRegex(
        R"(void\s+mainImage\s*\(\s*out\s+vec4\s+(\w+)\s*,\s*(?:in\s+)?vec2\s+(\w+)\s*\))");

    std::smatch match;
    std::string fragColorVar = "fragColor";
    std::string fragCoordVar = "fragCoord";

    // Najít názvy proměnných v mainImage
    if (std::regex_search(result, match, mainImageRegex)) {
        fragColorVar = match[1].str();
        fragCoordVar = match[2].str();
    }

    // Konvertovat fragCoord parametr na gl_FragCoord nebo vFragCoord
    // ShaderToy shadery očekávají pixelové souřadnice, ne normalizované
    std::string coordSource;
    if (target == ShaderTarget::WebGL1) {
        coordSource = "vFragCoord * iResolution.xy";
    } else {
        coordSource = "gl_FragCoord.xy";
    }

    // Vytvořit novou main funkci
    std::string newMainFunction;
    
    if (target == ShaderTarget::WebGL1) {
        newMainFunction = R"(
void main() {
    vec4 color;
    mainImage(color, )" + coordSource + R"();
    gl_FragColor = color;
})";
    } else {
        newMainFunction = R"(
void main() {
    mainImage(fragColor, )" + coordSource + R"();
})";
    }

    result += newMainFunction;
    return result;
}

std::string ShaderConvertor::convertShaderToyBuiltins(const std::string& code,
                                                      ShaderTarget target) {
    std::string result = code;

    // Nahrazení gl_FragCoord.xy za vFragCoord pro WebGL1
    if (target == ShaderTarget::WebGL1) {
        result = replaceAll(result, "gl_FragCoord.xy", "vFragCoord * iResolution.xy");
        result = replaceAll(result, "gl_FragCoord", "vec4(vFragCoord * iResolution.xy, 0.0, 1.0)");
    }

    // Konverze texture funkcí pro WebGL1
    if (target == ShaderTarget::WebGL1) {
        // Nahradit texture() za texture2D() pouze pokud ještě není
        std::regex textureRegex(R"(\btexture\s*\()");
        result = std::regex_replace(result, textureRegex, "texture2D(");
    }

    return result;
}

std::string ShaderConvertor::fixCompatibilityIssues(const std::string& code, ShaderTarget target) {
    std::string result = code;

    // Základní náhrady funkcí podle mapy náhrad
    if (functionReplacements.find(target) != functionReplacements.end()) {
        const auto& replacements = functionReplacements[target];
        for (const auto& pair : replacements) {
            result = replaceAll(result, pair.first, pair.second);
        }
    }

    // Specifické opravy pro různé platformy
    switch (target) {
    case ShaderTarget::WebGL1:
        result = fixWebGL1Issues(result);
        break;
    case ShaderTarget::WebGL2:
        result = fixWebGL2Issues(result);
        break;
    case ShaderTarget::Desktop330:
    case ShaderTarget::Desktop420:
        result = fixDesktopIssues(result);
        break;
    }

    return result;
}

std::string ShaderConvertor::fixWebGL1Issues(const std::string& code) {
    std::string result = code;

    // 1. Oprava radians() funkce
    std::regex radiansRegex(R"(radians\s*\(\s*([^)]+)\s*\))");
    result = std::regex_replace(result, radiansRegex, "($1 * 0.017453292519943295)");

    // 2. Oprava degrees() funkce
    std::regex degreesRegex(R"(degrees\s*\(\s*([^)]+)\s*\))");
    result = std::regex_replace(result, degreesRegex, "($1 * 57.295779513082320876798)");

    // 3. Oprava mod() pro negativní čísla
    std::regex modRegex(R"(\bmod\s*\(\s*([^,]+)\s*,\s*([^)]+)\s*\))");
    result = std::regex_replace(result, modRegex, "(($1) - ($2) * floor(($1) / ($2)))");

    // 4. Náhrada missing funkcí
    result = replaceAll(result, "textureLod(", "texture2DLodEXT(");
    result = replaceAll(result, "textureGrad(", "texture2DGradEXT(");

    return result;
}

std::string ShaderConvertor::fixWebGL2Issues(const std::string& code) {
    std::string result = code;
    
    // WebGL2 má méně problémů, ale některé věci stále potřebují úpravu
    // Většinou jen ověření, že texture2D() je nahrazeno za texture()
    result = replaceAll(result, "texture2D(", "texture(");
    
    return result;
}

std::string ShaderConvertor::fixDesktopIssues(const std::string& code) {
    std::string result = code;
    
    // Desktop OpenGL má nejméně problémů
    // Možné drobné úpravy podle potřeby
    
    return result;
}

std::string ShaderConvertor::applyAnalysisBasedFixes(const std::string& code, 
                                                     const ShaderAnalysis& analysis, 
                                                     ShaderTarget target) {
    std::string result = code;

    // Aplikovat opravy podle analýzy kódu
    if (analysis.hasMultiDeclarations) {
        result = fixMultiDeclarations(result);
    }

    if (analysis.hasComplexMath && target == ShaderTarget::WebGL1) {
        result = optimizeComplexMath(result);
    }

    return result;
}

std::string ShaderConvertor::fixMultiDeclarations(const std::string& code) {
    std::string result = code;
    
    // Příklad opravy: vec2 a = ..., b = ...; -> vec2 a = ...; vec2 b = ...;
    std::regex multiDeclRegex(R"((vec[2-4]|float|int)\s+(\w+)\s*=\s*([^,;]+)\s*,\s*(\w+)\s*=\s*([^;]+);)");
    
    while (std::regex_search(result, multiDeclRegex)) {
        result = std::regex_replace(result, multiDeclRegex, 
            "$1 $2 = $3;\n    $1 $4 = $5;", std::regex_constants::format_first_only);
    }
    
    return result;
}

std::string ShaderConvertor::optimizeComplexMath(const std::string& code) {
    std::string result = code;
    
    // Optimalizace pro WebGL1 - jednodušší matematické operace
    // Například nahrazení některých pow() voláních za explicitní násobení
    std::regex pow2Regex(R"(pow\s*\(\s*([^,]+)\s*,\s*2\.0?\s*\))");
    result = std::regex_replace(result, pow2Regex, "($1 * $1)");
    
    std::regex pow3Regex(R"(pow\s*\(\s*([^,]+)\s*,\s*3\.0?\s*\))");
    result = std::regex_replace(result, pow3Regex, "($1 * $1 * $1)");
    
    return result;
}

// === UTILITY IMPLEMENTATIONS ===

std::string ShaderConvertor::replaceAll(const std::string& str, const std::string& from, const std::string& to) {
    std::string result = str;
    size_t start_pos = 0;
    while ((start_pos = result.find(from, start_pos)) != std::string::npos) {
        result.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return result;
}

bool ShaderConvertor::containsFunction(const std::string& code, const std::string& functionName) {
    std::regex funcRegex("\\b" + functionName + "\\s*\\(");
    return std::regex_search(code, funcRegex);
}

ShaderAnalysis ShaderConvertor::analyzeShaderCode(const std::string& code) {
    ShaderAnalysis analysis;
    
    // Základní analýza texture kanálů
    std::regex channelRegex(R"(\biChannel([0-9]+)\b)");
    std::sregex_iterator iter(code.begin(), code.end(), channelRegex);
    std::sregex_iterator end;
    
    std::set<std::string> foundChannels;
    for (; iter != end; ++iter) {
        foundChannels.insert("iChannel" + iter->str(1));
    }
    
    analysis.textureChannels.assign(foundChannels.begin(), foundChannels.end());
    analysis.hasTextureChannels = !foundChannels.empty();
    
    // Analýza komplexních funkcí
    analysis.hasComplexMath = (code.find("pow(") != std::string::npos ||
                               code.find("exp(") != std::string::npos ||
                               code.find("log(") != std::string::npos);
    
    // Analýza multi-deklarací
    std::regex multiDeclRegex(R"((vec[2-4]|float|int)\s+\w+\s*=\s*[^,;]+\s*,\s*\w+\s*=)");
    analysis.hasMultiDeclarations = std::regex_search(code, multiDeclRegex);
    
    // Analýza smyček a podmínek
    analysis.hasLoops = (code.find("for(") != std::string::npos || 
                         code.find("while(") != std::string::npos);
    analysis.hasConditionals = (code.find("if(") != std::string::npos);
    
    // Přidání varování pro potenciální problémy
    if (analysis.hasComplexMath) {
        analysis.warnings.push_back("Complex mathematical functions detected - may need optimization for WebGL1");
    }
    
    return analysis;
}

void ShaderConvertor::initializeFunctionReplacements() {
    // WebGL1 náhrady (OpenGL ES 2.0)
    functionReplacements[ShaderTarget::WebGL1]["textureLod"] = "texture2DLodEXT";
    functionReplacements[ShaderTarget::WebGL1]["textureSize"] = "textureSize2D";
    functionReplacements[ShaderTarget::WebGL1]["inverse"] = "matrixInverse";
    functionReplacements[ShaderTarget::WebGL1]["transpose"] = "matrixTranspose";

    // WebGL2 má méně náhrad
    functionReplacements[ShaderTarget::WebGL2]["texture2D"] = "texture";

    // Desktop náhrady jsou minimální
    // Většinou není potřeba žádných náhrad pro moderní OpenGL
}

// === ADDITIONAL UTILITY METHODS ===

std::string ShaderConvertor::generateHeaderFile(const std::string& shaderToyCode, 
                                               const std::string& shaderName,
                                               const std::vector<ShaderTarget>& targets) {
    std::ostringstream headerStream;
    
    // Generate header preamble
    headerStream << "#ifndef __" << shaderName << "_H__\n";
    headerStream << "#define __" << shaderName << "_H__\n\n";
    
    // Generate shaders for each target
    for (const auto& target : targets) {
        auto result = convertFromShaderToy(shaderToyCode, target);
        if (result.success) {
            std::string suffix = getTargetSuffix(target);
            
            // Vertex shader
            headerStream << "const char* vertexShader" << suffix << " = R\"(\n";
            headerStream << result.vertexShader;
            headerStream << "\n)\";\n\n";
            
            // Fragment shader
            headerStream << "const char* fragmentShader" << suffix << " = R\"(\n";
            headerStream << result.fragmentShader;
            headerStream << "\n)\";\n\n";
        }
    }
    
    headerStream << "#endif // __" << shaderName << "_H__\n";
    return headerStream.str();
}

std::string ShaderConvertor::optimizeForTarget(const std::string& code, ShaderTarget target) {
    std::string result = code;
    
    switch (target) {
    case ShaderTarget::WebGL1:
        // Aggressive optimization for WebGL1
        result = optimizeComplexMath(result);
        result = simplifyPrecision(result);
        break;
    case ShaderTarget::WebGL2:
        // Moderate optimization
        result = optimizeComplexMath(result);
        break;
    case ShaderTarget::Desktop330:
    case ShaderTarget::Desktop420:
        // Minimal optimization for desktop
        break;
    }
    
    return result;
}

bool ShaderConvertor::validateShaderSyntax(const std::string& shaderCode, ShaderTarget target) {
    // Basic syntax validation - in real implementation would use OpenGL compiler
    // For now, just check for basic issues
    (void)target; // Suppress unused parameter warning
    
    // Check for unmatched braces
    int braceCount = 0;
    for (char c : shaderCode) {
        if (c == '{') braceCount++;
        else if (c == '}') braceCount--;
    }
    
    if (braceCount != 0) {
        return false;
    }
    
    // Check for required main function
    if (shaderCode.find("void main()") == std::string::npos) {
        return false;
    }
    
    return true;
}

std::string ShaderConvertor::getTargetInfo(ShaderTarget target) {
    switch (target) {
    case ShaderTarget::WebGL1:
        return "WebGL 1.0 (OpenGL ES 2.0)";
    case ShaderTarget::WebGL2:
        return "WebGL 2.0 (OpenGL ES 3.0)";
    case ShaderTarget::Desktop330:
        return "OpenGL 3.3 Core";
    case ShaderTarget::Desktop420:
        return "OpenGL 4.2 Core";
    default:
        return "Unknown";
    }
}

std::vector<std::string> ShaderConvertor::getSupportedExtensions(ShaderTarget target) {
    std::vector<std::string> extensions;
    
    switch (target) {
    case ShaderTarget::WebGL1:
        extensions = {"OES_texture_float", "OES_texture_float_linear", 
                     "EXT_texture_filter_anisotropic", "WEBGL_depth_texture"};
        break;
    case ShaderTarget::WebGL2:
        extensions = {"EXT_color_buffer_float", "OES_texture_float_linear"};
        break;
    case ShaderTarget::Desktop330:
    case ShaderTarget::Desktop420:
        extensions = {}; // Most extensions are core
        break;
    }
    
    return extensions;
}

bool ShaderConvertor::supportsFeature(ShaderTarget target, const std::string& feature) {
    // Feature support matrix
    if (feature == "textureLod") {
        return target != ShaderTarget::WebGL1; // Needs extension in WebGL1
    } else if (feature == "textureSize") {
        return target != ShaderTarget::WebGL1;
    } else if (feature == "precision") {
        return target == ShaderTarget::WebGL1 || target == ShaderTarget::WebGL2;
    }
    
    return true; // Default assume supported
}

// === PRIVATE HELPER METHODS ===

std::string ShaderConvertor::getTargetSuffix(ShaderTarget target) {
    switch (target) {
    case ShaderTarget::WebGL1:
        return "WebGL1";
    case ShaderTarget::WebGL2:
        return "WebGL2";
    case ShaderTarget::Desktop330:
        return "Desktop330";
    case ShaderTarget::Desktop420:
        return "Desktop420";
    default:
        return "Unknown";
    }
}

std::string ShaderConvertor::simplifyPrecision(const std::string& code) {
    std::string result = code;
    
    // Replace highp with mediump for WebGL1 compatibility
    result = replaceAll(result, "highp ", "mediump ");
    result = replaceAll(result, "precision highp", "precision mediump");
    
    return result;
}

// === NAMESPACE UTILITY FUNCTIONS ===

namespace ShaderUtils {
    std::string getShaderTargetString(ShaderTarget target) {
        switch (target) {
        case ShaderTarget::WebGL1:
            return "WebGL1";
        case ShaderTarget::WebGL2:
            return "WebGL2";
        case ShaderTarget::Desktop330:
            return "Desktop330";
        case ShaderTarget::Desktop420:
            return "Desktop420";
        default:
            return "Unknown";
        }
    }

    ShaderTarget parseShaderTarget(const std::string& targetStr) {
        if (targetStr == "WebGL1") return ShaderTarget::WebGL1;
        if (targetStr == "WebGL2") return ShaderTarget::WebGL2;
        if (targetStr == "Desktop330") return ShaderTarget::Desktop330;
        if (targetStr == "Desktop420") return ShaderTarget::Desktop420;
        return ShaderTarget::Desktop330; // Default
    }

    bool isWebGLTarget(ShaderTarget target) {
        return target == ShaderTarget::WebGL1 || target == ShaderTarget::WebGL2;
    }

    bool isDesktopTarget(ShaderTarget target) {
        return target == ShaderTarget::Desktop330 || target == ShaderTarget::Desktop420;
    }

    std::string getGLSLVersionString(ShaderTarget target) {
        switch (target) {
        case ShaderTarget::WebGL1:
            return ""; // No version directive
        case ShaderTarget::WebGL2:
            return "#version 300 es";
        case ShaderTarget::Desktop330:
            return "#version 330 core";
        case ShaderTarget::Desktop420:
            return "#version 420 core";
        default:
            return "#version 330 core";
        }
    }
}