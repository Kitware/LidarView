#include "vtkHelper.h"

#include <vtkPolyLine.h>

vtkSmartPointer<vtkPolyLine> CreatePolyLineFromPoints(const vtkSmartPointer<vtkPoints> & points)
{
  // Create new poly line with all points contains in the PolyData
  vtkSmartPointer<vtkPolyLine> polyLine = vtkSmartPointer<vtkPolyLine>::New();
  vtkIdType nbTotalPoints = points->GetNumberOfPoints();
  vtkIdList* polyIds = polyLine->GetPointIds();
  polyIds->Allocate(nbTotalPoints);
  for(vtkIdType i = 0; i < nbTotalPoints; i++)
  {
    polyIds->InsertNextId(i);
  }
  return polyLine;
}
