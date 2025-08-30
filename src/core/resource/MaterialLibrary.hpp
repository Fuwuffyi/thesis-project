#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

class Material;
class MaterialTemplate;

class MaterialLibrary final {
public:
   // Template management
   void RegisterTemplate(std::unique_ptr<MaterialTemplate> materialTemplate);
   const MaterialTemplate* GetTemplate(const std::string& typeName) const;
   bool HasTemplate(const std::string& typeName) const;
   std::vector<std::string> GetTemplateNames() const;

   // Pre-defined templates
   static void RegisterStandardTemplates(MaterialLibrary& library);
private:
   std::unordered_map<std::string, std::unique_ptr<MaterialTemplate>> m_templates;
};


