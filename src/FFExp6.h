/*******************************************************************************
GPU OPTIMIZED MONTE CARLO (GOMC) 2.31
Copyright (C) 2018  GOMC Group
A copy of the GNU General Public License can be found in the COPYRIGHT.txt
along with this program, also can be found at <http://www.gnu.org/licenses/>.
********************************************************************************/
#ifndef FF_EXP6_H
#define FF_EXP6_H

#include "EnsemblePreprocessor.h" //For "MIE_INT_ONLY" preprocessor.
#include "FFConst.h" //constants related to particles.
#include "BasicTypes.h" //for uint
#include "NumLib.h" //For Cb, Sq
#include "FFParticle.h"

//////////////////////////////////////////////////////////////////////
//////////////////////////// Exp-6 Style /////////////////////////////
//////////////////////////////////////////////////////////////////////
// Virial and LJ potential calculation:
//U(rij)= expConst * ( (6/alpha) * exp( alpha * [1-(r/rmin)] )-(rmin/r)^6)
//expConst=( eps-ij * alpha )/(alpha-6)
//Vir(r)= 6 * expConst * [ (r/rmin) * exp( alpha *[1-(r/rmin)]-(rmin/r)^6]
//
// Eelect = qi * qj * (1/r - 1/rcut)
// Welect = qi * qj * 1/rij^3


struct FF_EXP6 : public FFParticle {
public:
  FF_EXP6(Forcefield &ff) : FFParticle(ff), expConst(NULL), rMin(NULL),
    rMaxSq(NULL), expConst_1_4(NULL), rMin_1_4(NULL), rMaxSq_1_4(NULL) {}
  virtual ~FF_EXP6()
  {
    delete[] expConst;
    delete[] expConst_1_4;
    delete[] rMin;
    delete[] rMin_1_4;
    delete[] rMaxSq;
    delete[] rMaxSq_1_4;
  }

  virtual void Init(ff_setup::Particle const& mie,
                    ff_setup::NBfix const& nbfix);

  virtual double CalcEn(const double distSq,
                        const uint kind1, const uint kind2) const;
  virtual double CalcVir(const double distSq,
                         const uint kind1, const uint kind2) const;
  virtual void CalcAdd_1_4(double& en, const double distSq,
                           const uint kind1, const uint kind2) const;

  // coulomb interaction functions
  virtual double CalcCoulomb(const double distSq,
                             const double qi_qj_Fact, const uint b) const;
  virtual double CalcCoulombVir(const double distSq,
                                const double qi_qj, const uint b) const;
  virtual void CalcCoulombAdd_1_4(double& en, const double distSq,
                                  const double qi_qj_Fact,
                                  const bool NB) const;

  //!Returns energy correction
  virtual double EnergyLRC(const uint kind1, const uint kind2) const
  {
    return enCorrection[FlatIndex(kind1, kind2)];
  }
  //!!Returns virial correction
  virtual double VirialLRC(const uint kind1, const uint kind2) const
  {
    return virCorrection[FlatIndex(kind1, kind2)];
  }

  protected:

  double *expConst, *expConst_1_4, *rMaxSq, *rMin, *rMaxSq_1_4, *rMin_1_4;

};

inline void FF_EXP6::Init(ff_setup::Particle const& mie,
                          ff_setup::NBfix const& nbfix)
{
  //Initializ sigma and epsilon
  FFParticle::Init(mie, nbfix);
  uint size = num::Sq(count);
  //allocate memory 
  expConst = new double [size];
  rMin = new double [size];
  rMaxSq = new double [size];
  expConst_1_4 = new double [size];
  rMin_1_4 = new double [size];
  rMaxSq_1_4 = new double [size];
  //calculate exp-6 parameter
  for(uint i = 0; i < count; ++i) {
    for(uint j = 0; j < count; ++j) {
      uint idx = FlatIndex(i, j);
      //We use n as alpha for exp-6, with geometric combining
      expConst[idx] = epsilon[idx] * n[idx] / (n[idx] - 6.0);
      expConst_1_4[idx] = epsilon_1_4[idx] * n_1_4[idx] / (n_1_4[idx] - 6.0);

      //Find the Rmin(well depth) 
      double sigma = sqrt(sigmaSq[idx]);
      num::Exp6Fun* fun1 = new num::RminFun(n[idx], sigma);
      rMin[idx] = num::Zbrent(fun1, sigma, 3.0 * sigma, 1.0e-7);
      //Find the Rmax(du/dr = 0) 
      num::Exp6Fun* fun2 = new num::RmaxFun(n[idx], sigma, rMin[idx]);
      double rmax = Zbrent(fun2, 0.0, sigma, 1.0e-7);
      rMaxSq[idx] = rmax * rmax;

      //Find the Rmin(well depth) 
      double sigma_1_4 = sqrt(sigmaSq_1_4[idx]);
      num::Exp6Fun* fun3 = new num::RminFun(n_1_4[idx], sigma_1_4);
      rMin_1_4[idx] = num::Zbrent(fun3, sigma_1_4, 3.0 * sigma_1_4, 1.0e-7);
      //Find the Rmax(du/dr = 0) 
      num::Exp6Fun* fun4 = new num::RmaxFun(n_1_4[idx], sigma_1_4, rMin_1_4[idx]);
      double rmax_1_4 = Zbrent(fun4, 0.0, sigma_1_4, 1.0e-7);
      rMaxSq_1_4[idx] = rmax_1_4 * rmax_1_4;

      delete fun1;
      delete fun2;
      delete fun3;
      delete fun4; 
    }
  }
}
                  

inline void FF_EXP6::CalcAdd_1_4(double& en, const double distSq,
                                  const uint kind1, const uint kind2) const
{
  uint idx = FlatIndex(kind1, kind2);
  if(distSq < rMaxSq_1_4[idx]) {
    en += num::BIGNUM;
    return;
  }

  double dist = sqrt(distSq);
  double rRat = rMin_1_4[idx] / dist;
  double rRat2 = rRat * rRat;
  double attract = rRat2 * rRat2 * rRat2;
  double alpha_ij = n_1_4[idx];
  double repulse = (6.0 / alpha_ij) * exp(alpha_ij * 
                  (1.0 - dist/rMin_1_4[idx]));

  en += expConst_1_4[idx] * (repulse - attract);
}

inline void FF_EXP6::CalcCoulombAdd_1_4(double& en, const double distSq,
    const double qi_qj_Fact,
    const bool NB) const
{
  double dist = sqrt(distSq);
  if(NB)
    en += qi_qj_Fact / dist;
  else
    en += qi_qj_Fact * forcefield.scaling_14 / dist;
}


//mie potential
inline double FF_EXP6::CalcEn(const double distSq,
                               const uint kind1, const uint kind2) const
{
  if(forcefield.rCutSq < distSq)
    return 0.0;

  uint idx = FlatIndex(kind1, kind2);
  if(distSq < rMaxSq[idx]) {
    return num::BIGNUM;
  }

  double dist = sqrt(distSq);
  double rRat = rMin[idx] / dist;
  double rRat2 = rRat * rRat;
  double attract = rRat2 * rRat2 * rRat2;
  
  uint alpha_ij = n[idx];
  double repulse = (6.0 / alpha_ij) * exp(alpha_ij * 
                   (1.0 - dist/rMin[idx]));
  return expConst[idx] * (repulse - attract);
}

inline double FF_EXP6::CalcCoulomb(const double distSq,
                                    const double qi_qj_Fact, const uint b) const
{
  if(forcefield.rCutCoulombSq[b] < distSq)
    return 0.0;

  if(forcefield.ewald) {
    double dist = sqrt(distSq);
    double val = forcefield.alpha[b] * dist;
    return  qi_qj_Fact * erfc(val) / dist;
  } else {
    double dist = sqrt(distSq);
    return  qi_qj_Fact / dist;
  }
}

//mie potential
inline double FF_EXP6::CalcVir(const double distSq,
                                const uint kind1, const uint kind2) const
{
  if(forcefield.rCutSq < distSq)
    return 0.0;

  uint idx = FlatIndex(kind1, kind2);

  if(distSq < rMaxSq[idx]) {
    return num::BIGNUM;
  }

  double dist = sqrt(distSq);
  double rRat = rMin[idx] / dist;
  double rRat2 = rRat * rRat ;
  double attract = rRat2 * rRat2 * rRat2;

  uint alpha_ij = n[idx];
  double repulse = (dist / rMin[idx]) * exp(alpha_ij * 
                   (1.0 - dist / rMin[idx]));

  //Virial = -r * du/dr
  return 6.0 * expConst[idx] * (repulse - attract); 
}

inline double FF_EXP6::CalcCoulombVir(const double distSq,
                                       const double qi_qj, const uint b) const
{
  if(forcefield.rCutCoulombSq[b] < distSq)
    return 0.0;

  if(forcefield.ewald) {
    double dist = sqrt(distSq);
    double constValue = 2.0 * forcefield.alpha[b] / sqrt(M_PI);
    double expConstValue = exp(-1.0 * forcefield.alphaSq[b] * distSq);
    double temp = erfc(forcefield.alpha[b] * dist);
    return  qi_qj * (temp / dist + constValue * expConstValue) / distSq;
  } else {
    double dist = sqrt(distSq);
    return qi_qj / (distSq * dist);
  }
}


#endif /*FF_EXP6_H*/
