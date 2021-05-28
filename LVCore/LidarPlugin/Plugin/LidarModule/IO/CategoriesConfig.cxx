#include "CategoriesConfig.h"

#include <yaml-cpp/yaml.h>


//-----------------------------------------------------------------------------
// Helpers
bool robustReadYamlValueAsBool(YAML::Node node)
{
  bool ret;
  try
  {
    ret = node.as<bool>();
  }
  catch (YAML::BadConversion&)
  {
    int intRet = node.as<int>();
    ret = (intRet != 0);
  }
  return ret;

}

//-----------------------------------------------------------------------------
CategoriesConfig::CategoriesConfig(std::string filename)
{
  this->FileName = filename;
  bool ret = this->LoadCategories();
  if (ret)
  {
    std::cout << "Successfully loaded " << this->NbCategories << " categories." << std::endl;
  }
}


//-----------------------------------------------------------------------------
// Setters
void CategoriesConfig::SetFileName(std::string filename)
{
  this->FileName = filename;
}

// Getters
int CategoriesConfig::GetNbCategories()
{
  return this->NbCategories;
}

std::string CategoriesConfig::GetCategoryName(int categoryId)
{
  return this->CategoriesById[categoryId]["name"].as<std::string>();
}

bool CategoriesConfig::IsCategoryIgnored(int categoryId)
{
  return robustReadYamlValueAsBool(this->CategoriesById[categoryId]["ignore"]);
}

bool CategoriesConfig::IsCategoryThing(int categoryId)
{
  return robustReadYamlValueAsBool(this->CategoriesById[categoryId]["isthing"]);
}

std::vector<double> CategoriesConfig::GetCategoryColor(int categoryId)
{
  return this->CategoriesById[categoryId]["color"].as<std::vector<double>>();
}

std::string CategoriesConfig::GetSuperCategory(int categoryId)
{
  return this->CategoriesById[categoryId]["supercategory"].as<std::string>();
}

int CategoriesConfig::GetCategoryIdFromName(std::string name)
{
  return this->NameToIdMap[name];
}

int CategoriesConfig::GetCategoryIdFromExternalName(std::string name, std::string key)
{
  int catId = -1;
  for ( auto const& cat : this->CategoriesById )
  {
    YAML::Node potentialNames = cat.second["externalnames"][key];
    for ( auto const& potName: potentialNames)
    {
      if (potName.as<std::string>() == name)
      {
        catId = cat.first;
        break;
      }
    }
  }
  return catId ;
}

//-----------------------------------------------------------------------------
bool CategoriesConfig::LoadCategories()
{
  if (this->FileName.empty())
  {
    std::cout << "You need to provide a FileName to the object before loading categories";
    return false;
  }

  YAML::Node cats = YAML::LoadFile(this->FileName)["categories"];
  this->NbCategories = cats.size();

  // Fill maps
  this->CategoriesById = {};
  for (size_t i = 0; i < cats.size(); ++i)
  {
    this->CategoriesById.insert({cats[i]["id"].as<int>(), cats[i]});
    this->NameToIdMap.insert({cats[i]["name"].as<std::string>(), cats[i]["id"].as<int>()});
  }


  return true;
}
