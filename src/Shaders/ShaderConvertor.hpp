#ifndef SHADERCONVERTOR_HPP
#define SHADERCONVERTOR_HPP

#include <string>
#include <unordered_map>
#include <vector>

enum class ShaderTarget {
  WebGL1,     // OpenGL ES 2.0
  WebGL2,     // OpenGL ES 3.0
  Desktop330, // OpenGL 3.3
  Desktop420  // OpenGL 4.2+
};

struct ShaderConversionResult {
  std::string vertexShader;
  std::string fragmentShader;
  bool success = false;
  std::string errorMessage;
};

class ShaderConvertor {
public:
  ShaderConvertor ();
  ~ShaderConvertor ();

  // Hlavní funkce pro konverzi ShaderToy shaderu
  ShaderConversionResult convertFromShaderToy (const std::string& shaderToyCode,
                                               ShaderTarget target = ShaderTarget::Desktop330);

  // Statické funkce pro generování základních vertex shaderů
  static std::string getVertexShader (ShaderTarget target);

  // Funkce pro vytvoření C++ header souboru s vícero verzemi shaderu
  std::string generateHeaderFile (const std::string& shaderToyCode, const std::string& shaderName,
                                  const std::vector<ShaderTarget>& targets
                                  = { ShaderTarget::WebGL1, ShaderTarget::WebGL2,
                                      ShaderTarget::Desktop330 });

  // Funkce pro validaci a analýzu ShaderToy kódu
  struct ShaderAnalysis {
    bool hasComplexMath = false;
    bool hasMultiDeclarations = false;
    bool hasNonStandardParams = false;
    bool hasAdvancedGLSL = false;
    bool hasTextureChannels = false;
    bool hasCustomFunctions = false;
    std::vector<std::string> warnings;
    std::vector<std::string> usedFunctions;
    std::vector<std::string> textureChannels;
    std::vector<std::string> customFunctions;
  };

  ShaderAnalysis analyzeShaderCode (const std::string& code);

private:
  // Konverze hlavičky shaderu podle cílové platformy
  std::string convertShaderHeader (ShaderTarget target);

  // Konverze uniform proměnných
  std::string convertUniforms (const std::string& code, ShaderTarget target);

  // Konverze main funkce a výstupu
  std::string convertMainFunction (const std::string& code, ShaderTarget target);

  // Konverze built-in proměnných ShaderToy na standardní OpenGL
  std::string convertShaderToyBuiltins (const std::string& code, ShaderTarget target);

  // Nahrazení nekompatibilních funkcí
  std::string fixCompatibilityIssues (const std::string& code, ShaderTarget target);

  // Přidání precision specifiers pro mobilní platformy
  std::string addPrecisionSpecifiers (const std::string& code);

  // Utility funkce pro string manipulaci
  std::string replaceAll (const std::string& str, const std::string& from, const std::string& to);
  bool containsFunction (const std::string& code, const std::string& functionName);

  // Mapa náhrad pro různé funkce podle platformy
  std::unordered_map<ShaderTarget, std::unordered_map<std::string, std::string> >
      functionReplacements;

  // Inicializace náhrad funkcí
  void initializeFunctionReplacements ();
};

#endif