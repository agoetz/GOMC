
/*******************************************************************************
GPU OPTIMIZED MONTE CARLO (GOMC) 2.51
Copyright (C) 2018  GOMC Group
A copy of the GNU General Public License can be found in the COPYRIGHT.txt
along with this program, also can be found at <http://www.gnu.org/licenses/>.
********************************************************************************/
#ifndef ParallelTemperingUtilities_H
#define ParallelTemperingUtilities_H

#include "GOMC_Config.h"    //For version number
#if GOMC_LIB_MPI
#include <mpi.h>
#endif
#include "XYZArray.h"
#include "ParallelTemperingPreprocessor.h"
#include "System.h"
#include "ConfigSetup.h"

class ParallelTemperingUtilities
{
public:

#if GOMC_LIB_MPI
explicit ParallelTemperingUtilities(MultiSim const*const& multisim, FILE * fplog, System & sys, StaticVals const& statV, config_setup::Step stepData);
vector<bool> evaluateExchangeCriteria(ulong step);
void exchangePositions(Coordinates & myPos, MultiSim const*const& multisim, int exchangePartner, bool leader);
void exchangeCOMs(COM & myCOMs, MultiSim const*const& multisim, int exchangePartner, bool leader);
void exchangeCellLists(CellList & myCellList, MultiSim const*const& multisim, int exchangePartner, bool leader);
void exchangePotentials(SystemPotential & mySystemPotential, MultiSim const*const& multisim, int exchangePartner, bool leader);
void exchangeVirials(SystemPotential & mySystemPotential, MultiSim const*const& multisim, int exchangePartner, bool leader);
void conductExchanges(Coordinates & coords, COM & coms, MultiSim const*const& ms, vector<bool> & resultsOfExchangeCriteria);
void print_ind(FILE* fplog, const char* leg, int n, vector<int> ind, const vector<bool> bEx);
void print_prob(FILE* fplog, const char* leg, int n, vector<double> prob);
void print_count(FILE* fplog, const char* leg, int n, vector<int> count);
void print_transition_matrix(FILE* fplog, int n, vector<vector<int>> nmoves, const vector<int> nattempt);
void print_replica_exchange_statistics(FILE* fplog);

#endif

private:

FILE * fplog;
MultiSim const*const& ms;
PRNG & prng;
SystemPotential & sysPotRef;
SystemPotential sysPotNew;
ulong parallelTempFreq;
vector<double> global_betas;

vector<int> ind, pind;
vector<bool> exchangeResults;
vector<double> exchangeProbabilities;
vector<int> nexchange;

vector<int> nattempt;
//! Sum of probabilities
vector<double> prob_sum;
//! Number of moves between replicas i and j
vector<vector<int>> nmoves;

Coordinates newMolsPos;
COM newCOMs;

#if BOX_TOTAL == 1    
    vector<double> global_energies;
#else
    vector<vector <double> > global_energies;
#endif

};

#endif /*ParallelTemperingUtilities_H*/
