/****************************************************************************
 *
 * Pointwise Plugin utility functions
 *
 * Proprietary software product of Pointwise, Inc.
 * Copyright (c) 1995-2016 Pointwise, Inc.
 * All rights reserved.
 *
 ***************************************************************************/

#ifndef _RTCAEPSUPPORTDATA_H_
#define _RTCAEPSUPPORTDATA_H_

#include "COOLFluiDConstants.h"

CAEP_BCINFO CaeUnsCOOLFluiDBCInfo[] = {
    { "CatalyticWall", BcCatalyticWall },
    { "Dirichlet", BcDirichlet },
    { "FarField", BcFarField },
    { "Neumann", BcNeumann },
    { "NoSlipWall", BcNoSlipWall },
    { "Periodic", BcPeriodic },
    { "RadiativeWall", BcRadiativeWall },
    { "SlipWall", BcSlipWall },
    { "SubInlet", BcSubInlet },
    { "SubOutlet", BcSubOutlet },
    { "SuperInlet", BcSuperInlet },
    { "SuperOutlet", BcSuperOutlet },
    { "Symmetry", BcSymmetry },
};


//CAEP_VCINFO CaeUnsCOOLFluiDVCInfo[] = {
//    { "viscous-CaeUnsCOOLFluiD", 200 },
//    { "invisid-CaeUnsCOOLFluiD", 201 },
//};


const char *CaeUnsCOOLFluiDFileExt[] = {
    "CFmesh"
};

#endif /* _RTCAEPSUPPORTDATA_H_ */
