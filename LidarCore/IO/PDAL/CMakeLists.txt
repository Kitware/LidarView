vtk_module_find_package(PACKAGE PDAL VERSION 2.1)

set(classes
  vtkLASWriter)

set(pdal_libraries)
if (PDAL_VERSION VERSION_LESS "2.6")
  set(pdal_libraries pdal_util)
  if (APPLE OR WIN32)
    list(APPEND pdal_libraries
      pdalcpp)
  else()
    list(APPEND pdal_libraries
      pdal_base)
  endif()
else ()
  list(APPEND pdal_libraries
    pdalcpp)
endif ()

vtk_module_add_module(LidarView::IOPDAL
  CLASSES ${classes})
vtk_module_link(LidarView::IOPDAL
  PRIVATE
    ${pdal_libraries})
