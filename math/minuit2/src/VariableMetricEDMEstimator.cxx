// @(#)root/minuit2:$Id$
// Authors: M. Winkler, F. James, L. Moneta, A. Zsenei   2003-2005

/**********************************************************************
 *                                                                    *
 * Copyright (c) 2005 LCG ROOT Math team,  CERN/PH-SFT                *
 *                                                                    *
 **********************************************************************/

#include "Minuit2/VariableMetricEDMEstimator.h"
#include "Minuit2/FunctionGradient.h"
#include "Minuit2/MinimumError.h"

namespace ROOT {

namespace Minuit2 {

double VariableMetricEDMEstimator::Estimate(const FunctionGradient &g, const MinimumError &e) const
{
   // estimate the edm (expected distance to the minimum) =  0.5 * g^T V g  (where V is the error matrix, inverse of
   // Hessian)
   // edm = 0.5 * dx^T H dx   where V = H^-1 and dx is x distance from minimum
   // now since dx = - V g  -> edm = 0.5 g^T V g

   if (e.InvHessian().size() == 1)
      return 0.5 * g.Grad()(0) * g.Grad()(0) * e.InvHessian()(0, 0);

   double rho = similarity(g.Grad(), e.InvHessian());
   return 0.5 * rho;
}

} // namespace Minuit2

} // namespace ROOT
