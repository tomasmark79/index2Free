#include "ShaderConvertor.hpp"
#include <regex>
#include <sstream>
#include <algorithm>

ShaderConvertor::ShaderConvertor () {
  initializeFunctionReplacements ();
}

ShaderConvertor::~ShaderConvertor () {
}

ShaderConversionResult ShaderConvertor::convertFromShaderToy (const std::string& shaderToyCode,
                                                              ShaderTarget target) {
  ShaderConversionResult result;

  try {
    // 1. Získání vertex shaderu
    result.vertexShader = getVertexShader (target);

    // 2. Začátek s hlavičkou fragment shaderu
    std::string fragmentCode = convertShaderHeader (target);

    // 3. Přidání uniformů
    fragmentCode += convertUniforms (shaderToyCode, target);

    // 4. Konverze ShaderToy built-ins
    std::string processedCode = convertShaderToyBuiltins (shaderToyCode, target);

    // 5. Oprava kompatibility funkcí
    processedCode = fixCompatibilityIssues (processedCode, target);

    // 6. Přidání precision specifiers pro mobilní platformy - POUZE pokud už nejsou v convertUniforms
    if (target == ShaderTarget::WebGL1 || target == ShaderTarget::WebGL2) {
      // Pro WebGL už máme precision v convertUniforms(), takže přeskočíme
    } else {
      // Pro desktop platformy můžeme přidat precision jako fallback
      processedCode = addPrecisionSpecifiers (processedCode);
    }

    // 7. Konverze main funkce
    processedCode = convertMainFunction (processedCode, target);

    fragmentCode += processedCode;

    result.fragmentShader = fragmentCode;
    result.success = true;

  } catch (const std::exception& e) {
    result.success = false;
    result.errorMessage = "Conversion failed: " + std::string (e.what ());
  }

  return result;
}

std::string ShaderConvertor::getVertexShader (ShaderTarget target) {
  switch (target) {
  case ShaderTarget::WebGL1:
    return R"(precision highp float;
attribute vec2 pos;
varying vec2 uv;
void main() {
   uv = pos * 0.5 + 0.5;
   gl_Position = vec4(pos, 0.0, 1.0);
})";

  case ShaderTarget::WebGL2:
    return R"(#version 300 es
precision highp float;
layout (location = 0) in vec2 pos;
out vec2 uv;
void main() {
   uv = pos * 0.5 + 0.5;
   gl_Position = vec4(pos, 0.0, 1.0);
})";

  case ShaderTarget::Desktop330:
  case ShaderTarget::Desktop420:
  default:
    return R"(#version 330 core
layout (location = 0) in vec2 pos;
out vec2 uv;
void main() {
   uv = pos * 0.5f + 0.5f;
   gl_Position = vec4(pos, 0.0f, 1.0f);
})";
  }
}

std::string ShaderConvertor::generateHeaderFile (const std::string& shaderToyCode,
                                                 const std::string& shaderName,
                                                 const std::vector<ShaderTarget>& targets) {
  std::stringstream ss;

  ss << "#ifndef __" << shaderName << "_H__\n";
  ss << "#define __" << shaderName << "_H__\n\n";

  for (const auto& target : targets) {
    auto result = convertFromShaderToy (shaderToyCode, target);

    if (!result.success) {
      ss << "// Error converting for target: " << result.errorMessage << "\n";
      continue;
    }

    std::string suffix;
    std::string comment;

    switch (target) {
    case ShaderTarget::WebGL1:
      suffix = "200";
      comment = "WebGL1 shaders (OpenGL ES 2.0)";
      break;
    case ShaderTarget::WebGL2:
      suffix = "300";
      comment = "WebGL2 shaders (OpenGL ES 3.0)";
      break;
    case ShaderTarget::Desktop330:
      suffix = "330";
      comment = "Desktop OpenGL shaders";
      break;
    case ShaderTarget::Desktop420:
      suffix = "420";
      comment = "Desktop OpenGL 4.2+ shaders";
      break;
    }

    ss << "// " << comment << "\n";
    ss << "const char* vertexShader" << suffix << " = R\"(";
    ss << result.vertexShader << ")\";\n\n";

    ss << "const char* fragmentShader" << suffix << " = R\"(";
    ss << result.fragmentShader << ")\";\n\n";
  }

  ss << "#endif // __" << shaderName << "_H__\n";

  return ss.str ();
}

std::string ShaderConvertor::convertShaderHeader (ShaderTarget target) {
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

std::string ShaderConvertor::convertUniforms (const std::string& code, ShaderTarget target) {
  std::string uniforms;

  // Pro WebGL/OpenGL ES musí být precision specifier na ZAČÁTKU, PŘED uniformy!
  if (target == ShaderTarget::WebGL1 || target == ShaderTarget::WebGL2) {
    uniforms += "precision mediump float;\n\n";
  }

  // Standardní ShaderToy uniformy
  uniforms += "uniform float iTime;\n";
  uniforms += "uniform vec2 iResolution;\n";

  // Volitelné uniformy (kontrola, jestli se používají v kódu)
  if (containsFunction (code, "iMouse")) {
    uniforms += "uniform vec4 iMouse;\n";
  }
  if (containsFunction (code, "iFrame")) {
    uniforms += "uniform int iFrame;\n";
  }
  if (containsFunction (code, "iDate")) {
    uniforms += "uniform vec4 iDate;\n";
  }

  // Input/Output proměnné podle verze
  switch (target) {
  case ShaderTarget::WebGL1:
    uniforms += "varying vec2 uv;\n\n";
    break;

  case ShaderTarget::WebGL2:
  case ShaderTarget::Desktop330:
  case ShaderTarget::Desktop420:
    uniforms += "in vec2 uv;\n";
    uniforms += "out vec4 fragColor;\n\n";
    break;
  }

  return uniforms;
}

std::string ShaderConvertor::convertMainFunction (const std::string& code, ShaderTarget target) {
  std::string result = code;

  // Pokročilejší regex pro různé varianty mainImage funkcí
  // Zachytí různé názvy parametrů jako O, fragColor, color atd.
  // Také podporuje případy bez 'in' kvalifikátoru
  std::regex mainImageRegex (
      R"(void\s+mainImage\s*\(\s*out\s+vec4\s+(\w+)\s*,\s*(?:in\s+)?vec2\s+(\w+)\s*\))");

  std::smatch match;
  std::string fragColorVar = "fragColor";
  std::string fragCoordVar = "fragCoord";

  // Najít názvy proměnných v mainImage
  if (std::regex_search (result, match, mainImageRegex)) {
    fragColorVar = match[1].str (); // např. "O", "fragColor", "color"
    fragCoordVar = match[2].str (); // např. "F", "fragCoord", "uv"
  }

  // 🎯 NOVÝ PŘÍSTUP: Necháme mainImage být a vytvoříme novou main() funkci
  // Tímto způsobem se vyhneme všem problémům s nahrazováním parametrů v custom funkcích

  std::string newMainFunction;

  if (target == ShaderTarget::WebGL1) {
    // Pro WebGL1 používáme gl_FragColor
    newMainFunction = R"(
void main() {
    vec4 color;
    mainImage(color, uv * iResolution);
    gl_FragColor = color;
})";
  } else {
    // Pro ostatní platformy používáme fragColor
    newMainFunction = R"(
void main() {
    mainImage(fragColor, uv * iResolution);
})";
  }

  // Přidáme novou main funkci na konec
  result += newMainFunction;

  return result;
}

std::string ShaderConvertor::convertShaderToyBuiltins (const std::string& code,
                                                       ShaderTarget target) {
  std::string result = code;

  // Konverze ShaderToy texture kanálů na standardní uniformy

  // Detekce použitých texture kanálů
  std::vector<std::string> channels = { "iChannel0", "iChannel1", "iChannel2", "iChannel3" };
  std::vector<std::string> usedChannels;

  for (const auto& channel : channels) {
    if (containsFunction (result, channel)) {
      usedChannels.push_back (channel);
    }
  }

  // Nahrazení texture(iChannelX, ...) za texture(texX, ...)
  for (size_t i = 0; i < usedChannels.size (); ++i) {
    std::string channel = usedChannels[i];
    std::string texName = "tex" + std::to_string (i);

    // Nahrazení texture calls - PŘESNĚ s mezerou nebo závorkou
    std::string searchPattern = "texture(" + channel;
    std::string replacePattern;

    if (target == ShaderTarget::WebGL1) {
      replacePattern = "texture2D(" + texName;
    } else {
      replacePattern = "texture(" + texName;
    }

    result = replaceAll (result, searchPattern, replacePattern);
  }

  // Přidat komentář o potřebných uniform deklaracích
  if (!usedChannels.empty ()) {
    std::string uniformDeclaration = "\n// Note: Add these uniform declarations:\n";
    for (size_t j = 0; j < usedChannels.size (); ++j) {
      uniformDeclaration
          += "// uniform sampler2D tex" + std::to_string (j) + "; // for " + usedChannels[j] + "\n";
    }
    result = uniformDeclaration + result;
  }

  return result;
}

std::string ShaderConvertor::fixCompatibilityIssues (const std::string& code, ShaderTarget target) {
  std::string result = code;

  // Nahrazení funkcí podle mapy náhrad
  if (functionReplacements.find (target) != functionReplacements.end ()) {
    const auto& replacements = functionReplacements[target];

    for (const auto& pair : replacements) {
      result = replaceAll (result, pair.first, pair.second);
    }
  }

  // ⚡ KRITICKÉ WEBGL1 OPRAVY
  if (target == ShaderTarget::WebGL1) {

    // 1. Oprava array syntaxe - vec4 [2] není podporováno v WebGL1
    std::regex arrayDeclRegex (R"((\w+)\s*\[\s*(\d+)\s*\]\s*(\w+)\s*=\s*\1\[\2\]\s*\([^)]+\))");
    std::string arrayReplacement = "$1 $3_0, $3_1; $3_0 = $1(0.); $3_1 = $1(0.)";
    result = std::regex_replace (result, arrayDeclRegex, arrayReplacement);

    // 2. Náhrada isnan() a isinf() - nejsou v WebGL1 ES
    result = replaceAll (result, "isnan(", "false && (");
    result = replaceAll (result, "isinf(", "false && (");

    // 3. DEAKTIVOVÁNO: Precision specifier už byl přidán v convertUniforms()
    // if (result.find("precision mediump float;") == std::string::npos) {
    //   result = "precision mediump float;\n" + result;
    // }

    // 4. Oprava komplexních for cyklů s float iterators
    std::regex floatForRegex (R"(for\s*\(\s*float\s+(\w+)\s*=\s*([^;]+);\s*([^;]+);\s*([^)]+)\))");
    result = std::regex_replace (
        result, floatForRegex,
        "for(int $1_i = int($2); $1_i < int($3); $1_i++) { float $1 = float($1_i);");

    // 5. Přidání warning komentáře pro složité shadery
    if (result.length () > 20000) {
      result = "// WARNING: This shader is very complex for WebGL1 - performance issues expected\n"
               + result;
    }
  }

  // Specifické opravy pro pokročilé GLSL konstrukce

  // 1. Oprava mat2 konstruktoru s vec4 - převod na explicitní formu
  if (target == ShaderTarget::WebGL1) {
    // WebGL1 může mít problémy s některými mat2 konstruktory
    std::regex mat2Vec4Regex (R"(mat2\s*\(\s*cos\s*\([^)]+\)\s*\+\s*vec4\s*\([^)]+\)\s*\))");
    if (std::regex_search (result, mat2Vec4Regex)) {
      // Pro složitější konstrukce s mat2 a vec4 ponecháme původní kód
      // ale přidáme komentář o možných problémech
      result
          = "// Warning: Complex mat2 constructor may need manual adjustment for WebGL1\n" + result;
    }
  }

  // 2. Oprava složitých for smyček s prázdnou inicializací
  // for(; i++<9.; w += 1.+sin(v) ) -> explicitnější forma
  std::regex emptyForRegex (R"(for\s*\(\s*;\s*([^;]+);\s*([^)]+)\s*\))");
  result = std::regex_replace (result, emptyForRegex, "for (int _loop = 0; $1; $2)");

  // 3. Oprava vícenásobných deklarací na jednom řádku
  // Necháme původní kód, ale přidáme komentář pro složité případy
  if (result.find ("vec2 r = iResolution.xy,") != std::string::npos) {
    // Komplexní multi-deklarace zůstávají, ale přidáme poznámku
    result
        = "// Note: Multiple variable declarations on one line - verify compatibility\n" + result;
  }

  // 4. Kontrola použití log() funkce - může vyžadovat polyfill pro starší platformy
  if (containsFunction (result, "log(") && target == ShaderTarget::WebGL1) {
    result = "// Note: log() function used - ensure proper precision on mobile\n" + result;
  }

  return result;
}

std::string ShaderConvertor::addPrecisionSpecifiers (const std::string& code) {
  // Kontrola, zda precision specifier už není přítomen
  if (code.find ("precision") != std::string::npos) {
    // Precision už je v kódu, nebudeme přidávat duplicitní
    return code;
  }

  std::string precision = "precision mediump float;\n";
  return precision + code;
}

std::string ShaderConvertor::replaceAll (const std::string& str, const std::string& from,
                                         const std::string& to) {
  std::string result = str;
  size_t pos = 0;

  while ((pos = result.find (from, pos)) != std::string::npos) {
    result.replace (pos, from.length (), to);
    pos += to.length ();
  }

  return result;
}

bool ShaderConvertor::containsFunction (const std::string& code, const std::string& functionName) {
  return code.find (functionName) != std::string::npos;
}

void ShaderConvertor::initializeFunctionReplacements () {
  // Inicializace náhrad funkcí pro různé platformy

  // WebGL1 náhrady (OpenGL ES 2.0)
  // POZOR: Nenahrazuj texture() obecně, protože convertShaderToyBuiltins to už udělala!
  // functionReplacements[ShaderTarget::WebGL1]["texture"] = "texture2D";  // VYPNUTO
  functionReplacements[ShaderTarget::WebGL1]["textureLod"] = "texture2DLodEXT";
  functionReplacements[ShaderTarget::WebGL1]["textureSize"]
      = "textureSize2D"; // Může vyžadovat extension

  // WebGL2 náhrady (méně problémové)
  // functionReplacements[ShaderTarget::WebGL2]["texture2D"] = "texture";  // VYPNUTO

  // Desktop náhrady jsou obvykle minimální
  // Většinou není potřeba žádných náhrad

  // Specifické náhrady pro komplexní konstrukce
  // (Pro budoucí rozšíření)
}

ShaderConvertor::ShaderAnalysis ShaderConvertor::analyzeShaderCode (const std::string& code) {
  ShaderAnalysis analysis;

  // Detekce complex math funkcí
  std::vector<std::string> complexFunctions
      = { "log", "exp", "pow", "sqrt", "sin", "cos", "atan", "length" };
  for (const auto& func : complexFunctions) {
    if (containsFunction (code, func + "(")) {
      analysis.hasComplexMath = true;
      analysis.usedFunctions.push_back (func);
    }
  }

  // Detekce multi-deklarací
  if (code.find (",\n         ") != std::string::npos
      || code.find ("vec2 r = iResolution.xy,") != std::string::npos) {
    analysis.hasMultiDeclarations = true;
    analysis.warnings.push_back ("Multiple variable declarations on single line detected");
  }

  // Detekce non-standard parametrů v mainImage
  std::regex nonStandardParams (
      R"(void\s+mainImage\s*\(\s*out\s+vec4\s+(\w+)\s*,\s*(?:in\s+)?vec2\s+(\w+)\s*\))");
  std::smatch match;
  if (std::regex_search (code, match, nonStandardParams)) {
    std::string colorVar = match[1].str ();
    std::string coordVar = match[2].str ();

    if (colorVar != "fragColor" || coordVar != "fragCoord") {
      analysis.hasNonStandardParams = true;
      analysis.warnings.push_back ("Non-standard parameter names: " + colorVar + ", " + coordVar
                                   + " (should be fragColor, fragCoord)");
    }
  }

  // Detekce texture kanálů
  std::vector<std::string> channels = { "iChannel0", "iChannel1", "iChannel2", "iChannel3" };
  for (const auto& channel : channels) {
    if (containsFunction (code, channel)) {
      analysis.hasTextureChannels = true;
      analysis.textureChannels.push_back (channel);
    }
  }

  if (analysis.hasTextureChannels) {
    analysis.warnings.push_back (
        "ShaderToy texture channels detected - will need manual uniform setup");
  }

  // Detekce custom funkcí (funkce definované před mainImage)
  std::regex customFuncRegex (R"(\b(float|vec[2-4]|mat[2-4]|int|bool)\s+(\w+)\s*\([^)]*\)\s*\{)");
  std::sregex_iterator iter (code.begin (), code.end (), customFuncRegex);
  std::sregex_iterator end;

  while (iter != end) {
    std::string funcName = iter->str (2);
    if (funcName != "mainImage" && funcName != "main") {
      analysis.hasCustomFunctions = true;
      analysis.customFunctions.push_back (funcName);
    }
    ++iter;
  }

  if (analysis.hasCustomFunctions) {
    analysis.warnings.push_back (
        "Custom function definitions detected - verify parameter conversions");
  }

  // Detekce advanced GLSL konstrukcí
  if (containsFunction (code, "mat2(") || containsFunction (code, "mat3(")
      || containsFunction (code, "mat4(")) {
    analysis.hasAdvancedGLSL = true;
    analysis.warnings.push_back ("Matrix constructors detected - verify compatibility");
  }

  // For smyčky s prázdnou inicializací
  if (code.find ("for(;") != std::string::npos) {
    analysis.hasAdvancedGLSL = true;
    analysis.warnings.push_back ("Empty for-loop initialization detected");
  }

  // Složité assignment výrazy
  if (code.find (" += ") != std::string::npos && code.find (" = -") != std::string::npos) {
    analysis.hasAdvancedGLSL = true;
    analysis.warnings.push_back ("Complex chained assignments detected (e.g., g += d = -...)");
  }

  return analysis;
}