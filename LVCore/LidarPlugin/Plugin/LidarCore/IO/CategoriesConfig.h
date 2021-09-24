#ifndef CATEGORIESCONFIG_H
#define CATEGORIESCONFIG_H

#include <string>
#include <unordered_map>

#include <yaml-cpp/yaml.h>

#include <vtkObject.h>

#include "LidarCoreModule.h"


/**
 * @brief The CategoriesConfig parses a categories configuration file and accesses its values

One categories config file can be created for each dataset.
This yaml file has the following structure:

categories:  # List of all the categories in a dataset
  - name: category1
    id: 1
    supercategory: supercategory1  # Higher level category (ie. group of categories) such as vehicle or animal
    color:  # Color of the pixels corresponding to the class for semantic segementation and visualisation
    - value for blue channel
    - value for green channel
    - value for red channel
    isthing: 1  # boolean set to 1 if the categoryrepresents objets that can be delimited by a bbox
    ignore: false  # boolean set to 1 if the category should be ignored in 3D Bboxes computation
    canmove: true  # boolean set to 1 if instances of the category can move (ex: person, vehicle)
    externalnames:  # dictionary of the names it has in different datasets used for 2D detection
      yolo: ["category1", "category2"]
      psp: ["category 1"]


 */
class LIDARCORE_EXPORT CategoriesConfig
{
public:
  CategoriesConfig() = default;
  ~CategoriesConfig(){};

  CategoriesConfig(std::string);

  // Setters
  void SetFileName(std::string);

  // Getters
  int GetNbCategories();
  std::string GetCategoryName(int);
  bool IsCategoryIgnored(int);
  bool IsCategoryThing(int);
  std::vector<double> GetCategoryColor(int);
  std::string GetSuperCategory(int);

  int GetCategoryIdFromName(std::string);
  int GetCategoryIdFromExternalName(std::string, std::string);

  bool LoadCategories();  // Returns true if categories have been loaded correctly


protected:
  //! yaml annotation file containing categories informations
  std::string FileName = "";
  int NbCategories = 0;
  std::unordered_map<int, YAML::Node> CategoriesById = {};
  std::unordered_map<std::string, int> NameToIdMap = {};

};

#endif // CATEGORIESCONFIG_H
