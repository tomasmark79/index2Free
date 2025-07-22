#ifndef SHADERCONVERTOR_HPP
#define SHADERCONVERTOR_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <regex>

enum class ShaderTarget {
  WebGL1,     // OpenGL ES 2.0
  WebGL2,     // OpenGL ES 3.0
  Desktop330, // OpenGL 3.3
  Desktop420  // OpenGL 4.2+
};

struct ShaderAnalysis {
  bool hasComplexMath = false;
  bool hasMultiDeclarations = false;
  bool hasNonStandardParams = false;
  bool hasAdvancedGLSL = false;
  bool hasTextureChannels = false;
  bool hasCustomFunctions = false;
  bool hasAudioFeatures = false;
  bool hasLoops = false;
  bool hasConditionals = false;

  std::vector<std::string> warnings;
  std::vector<std::string> errors;
  std::vector<std::string> usedFunctions;
  std::vector<std::string> textureChannels;
  std::vector<std::string> customFunctions;
  std::vector<std::string> uniformsUsed;
  std::vector<std::string> defines;

  int complexityScore = 0;
  size_t estimatedInstructions = 0;
};

struct ShaderConversionResult {
  std::string vertexShader;
  std::string fragmentShader;
  bool success = false;
  std::string errorMessage;
  ShaderAnalysis analysis;
  ShaderTarget targetUsed;

  // Dodatečné informace pro debugging
  std::vector<std::string> conversionWarnings;
  std::string originalCode;
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
  ShaderAnalysis analyzeShaderCode (const std::string& code);

  // Funkce pro optimalizaci shaderu podle cílové platformy
  std::string optimizeForTarget (const std::string& code, ShaderTarget target);

  // Validace výsledného shaderu
  bool validateShaderSyntax (const std::string& shaderCode, ShaderTarget target);

  // Utility funkce pro získání informací o platformě
  static std::string getTargetInfo (ShaderTarget target);
  static std::vector<std::string> getSupportedExtensions (ShaderTarget target);
  static bool supportsFeature (ShaderTarget target, const std::string& feature);

private:
  // === Core conversion methods ===

  // Konverze hlavičky shaderu podle cílové platformy
  std::string convertShaderHeader (ShaderTarget target);

  // Konverze uniform proměnných s analýzou
  std::string convertUniforms (const std::string& code, ShaderTarget target,
                               const ShaderAnalysis& analysis);

  // Konverze main funkce a výstupu
  std::string convertMainFunction (const std::string& code, ShaderTarget target);

  // Konverze built-in proměnných ShaderToy na standardní OpenGL
  std::string convertShaderToyBuiltins (const std::string& code, ShaderTarget target);

  // Nahrazení nekompatibilních funkcí
  std::string fixCompatibilityIssues (const std::string& code, ShaderTarget target);

  // === Platform-specific fixes ===

  std::string fixWebGL1Issues (const std::string& code);
  std::string fixWebGL2Issues (const std::string& code);
  std::string fixDesktopIssues (const std::string& code);

  // === Analysis-based optimizations ===

  std::string applyAnalysisBasedFixes (const std::string& code, const ShaderAnalysis& analysis,
                                       ShaderTarget target);
  std::string fixMultiDeclarations (const std::string& code);
  std::string optimizeComplexMath (const std::string& code);
  std::string simplifyLoops (const std::string& code);

  // === Analysis methods ===

  void analyzeUniforms (const std::string& code, ShaderAnalysis& analysis);
  void analyzeTextures (const std::string& code, ShaderAnalysis& analysis);
  void analyzeFunctions (const std::string& code, ShaderAnalysis& analysis);
  void analyzeComplexity (const std::string& code, ShaderAnalysis& analysis);
  void analyzeDefines (const std::string& code, ShaderAnalysis& analysis);

  // === Utility functions ===

  // String manipulation utilities
  std::string replaceAll (const std::string& str, const std::string& from, const std::string& to);
  bool containsFunction (const std::string& code, const std::string& functionName);
  std::vector<std::string> extractFunctionNames (const std::string& code);
  std::vector<std::string> extractUniformUsage (const std::string& code);
  std::string removeComments (const std::string& code);
  std::string normalizeWhitespace (const std::string& code);

  // Regex patterns for common shader constructs
  std::string getUniformRegexPattern ();
  std::string getFunctionRegexPattern ();
  std::string getTextureRegexPattern ();

  // === Header generation utilities ===

  std::string generateHeaderPreamble (const std::string& shaderName);
  std::string generateTargetSpecificCode (const std::string& code, ShaderTarget target,
                                          const std::string& suffix);
  std::string generateHeaderPostamble ();

  // === Validation utilities ===

  bool checkGLSLSyntax (const std::string& code, ShaderTarget target);
  std::vector<std::string> findPotentialIssues (const std::string& code, ShaderTarget target);
  bool isValidGLSLIdentifier (const std::string& identifier);

  // === Member variables ===

  // Mapa náhrad pro různé funkce podle platformy
  std::unordered_map<ShaderTarget, std::unordered_map<std::string, std::string> >
      functionReplacements;

  // Mapa podporovaných funkcí pro každou platformu
  std::unordered_map<ShaderTarget, std::vector<std::string> > supportedFunctions;

  // Mapa náhrad pro built-in proměnné
  std::unordered_map<ShaderTarget, std::unordered_map<std::string, std::string> >
      builtinReplacements;

  // Cache pro kompilované regex vzory
  mutable std::unordered_map<std::string, std::shared_ptr<std::regex> > regexCache;

  // === Initialization methods ===

  void initializeFunctionReplacements ();
  void initializeSupportedFunctions ();
  void initializeBuiltinReplacements ();
  void initializeRegexPatterns ();

  // === Internal helper methods ===

  std::shared_ptr<std::regex> getCompiledRegex (const std::string& pattern) const;
  std::string escapeRegexSpecialChars (const std::string& str);
  bool matchesPattern (const std::string& text, const std::string& pattern) const;

  // Target-specific helpers
  std::string getTargetSuffix (ShaderTarget target);
  std::string simplifyPrecision (const std::string& code);

  // Error handling
  void addWarning (ShaderConversionResult& result, const std::string& warning);
  void addError (ShaderConversionResult& result, const std::string& error);
};

// === Global utility functions ===

namespace ShaderUtils {
  // Utility funkce mimo třídu pro snadnější použití
  std::string getShaderTargetString (ShaderTarget target);
  ShaderTarget parseShaderTarget (const std::string& targetStr);
  bool isWebGLTarget (ShaderTarget target);
  bool isDesktopTarget (ShaderTarget target);
  std::string getGLSLVersionString (ShaderTarget target);
}

#endif // SHADERCONVERTOR_HPP