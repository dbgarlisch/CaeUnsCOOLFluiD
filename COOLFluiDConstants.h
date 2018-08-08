/****************************************************************************
 *
 * Pointwise Plugin utility functions
 *
 * Proprietary software product of Pointwise, Inc.
 * Copyright (c) 1995-2016 Pointwise, Inc.
 * All rights reserved.
 *
 ***************************************************************************/

#ifndef _COOLFLUIDCONSTANTS_H_
#define _COOLFLUIDCONSTANTS_H_

enum BCPhysicalTypes {
    BcCatalyticWall = 1,
    BcDirichlet     = 2,
    BcFarField      = 3,
    BcNeumann       = 4,
    BcNoSlipWall    = 5,
    BcPeriodic      = 6,
    BcRadiativeWall = 7,
    BcSlipWall      = 8,
    BcSubInlet      = 9,
    BcSubOutlet     = 10,
    BcSuperInlet    = 11,
    BcSuperOutlet   = 12,
    BcSymmetry      = 13
};

#endif /* _COOLFLUIDCONSTANTS_H_ */
