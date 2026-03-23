/*=========================================================================

  Program: LidarView
  Module:  vtkLiveSourceAlgorithm.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkLiveSourceAlgorithm_h
#define vtkLiveSourceAlgorithm_h

#include <vtkAlgorithm.h>

class vtkInformation;
class vtkInformationVector;
/**
 * @class   vtkLiveSourceAlgorithm
 * @brief   Base class for algorithms using LiveSource
 *
 * This class is designed for algorithms used with LiveSource, where RequestInformation
 * serves as an initialization method. Live sources typically call this->Modified() to
 * refresh their visualization, which triggers the RequestInformation process for both
 * the LiveSource and all downstream algorithms.
 *
 * To update a LiveSource efficiently within GetNeedUpdate(), call this->UpdateLiveSource()
 * instead of this->Modified().
 *
 * @warning Python wrapping of subclasses require special handling.
 * Here is an example ensuring wrapping works as expected,
 * implementing a temporal algorithm as a `vtkPassInputTypeAlgorithm`:
 *
 * ```
 * #include <vtkPassInputTypeAlgorithm.h>
 * #include <vtkTemporalAlgorithm.h>
 *
 * #ifndef __VTK_WRAP__
 * #define vtkPassInputTypeAlgorithm vtkTemporalAlgorithm<vtkPassInputTypeAlgorithm>
 * #endif
 *
 * We make the wrapper think that we inherit from vtkPassInputTypeAlgorithm
 * class MyLiveSourceFilter : vtkPassInputTypeAlgorithm
 * {
 * public:
 *   vtkTypeMacro(MyLiveSourceFilter, vtkPassInputTypeAlgorithm);
 *
 * We do not need to trick the wrapper with the superclass name anymore
 * #ifndef __VTK_WRAP__
 * #undef vtkPassInputTypeAlgorithm
 * #endif
 *
 * };
 * ```
 */
template <class AlgorithmT>
class vtkLiveSourceAlgorithm : public AlgorithmT
{
public:
  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  vtkTemplateTypeMacro(vtkLiveSourceAlgorithm, AlgorithmT);
  ///@}

  static_assert(std::is_base_of<vtkAlgorithm, AlgorithmT>::value,
    "Template argument must inherit vtkAlgorithm");

  /**
   * Updates the LiveSource without resetting the NeedInitialization flag.
   * Use this method instead of Modified() when working with LiveSource to
   * avoid unnecessary re-initialization.
   */
  void UpdateLiveSource();

  /**
   * Overrides the standard Modified method to ensure that initialization
   * processes are refreshed appropriately.
   */
  void Modified() override;

  /**
   * Process a request from the executive. Calling RequestInformation only
   * when necessary.
   */
  vtkTypeBool ProcessRequest(vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) override;

protected:
  vtkLiveSourceAlgorithm() = default;
  ~vtkLiveSourceAlgorithm() override = default;

private:
  vtkLiveSourceAlgorithm(const vtkLiveSourceAlgorithm&) = delete;
  void operator=(const vtkLiveSourceAlgorithm&) = delete;

  bool NeedInitialization = true;
};

#include "vtkLiveSourceAlgorithm.txx"
#endif
