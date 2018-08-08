/****************************************************************************
 *
 * class CaeUnsCOOLFluiD
 *
 * Proprietary software product of Pointwise, Inc.
 * Copyright (c) 1995-2016 Pointwise, Inc.
 * All rights reserved.
 *
 ***************************************************************************/

#ifndef _CAEUNSCOOLFLUID_H_
#define _CAEUNSCOOLFLUID_H_

#include "apiGridModel.h"
#include "apiPWP.h"

#include "CaePlugin.h"
#include "CaeUnsGridModel.h"


//***************************************************************************
//***************************************************************************
//***************************************************************************

class CaeUnsCOOLFluiD :
    public CaeUnsPlugin,
    public CaeFaceStreamHandler {

public:
    CaeUnsCOOLFluiD(CAEP_RTITEM *pRti, PWGM_HGRIDMODEL model,
        const CAEP_WRITEINFO *pWriteInfo);
    virtual ~CaeUnsCOOLFluiD();
    static bool create(CAEP_RTITEM &rti);
    static void destroy(CAEP_RTITEM &rti);

private:
    enum CalcType { FiniteVolume, FiniteElement };

    virtual bool        beginExport();
    virtual PWP_BOOL    write();
    virtual bool        endExport();

    // face streaming handlers
    virtual PWP_UINT32 streamBegin(const PWGM_BEGINSTREAM_DATA &data);
    virtual PWP_UINT32 streamFace(const PWGM_FACESTREAM_DATA &data);
    virtual PWP_UINT32 streamEnd(const PWGM_ENDSTREAM_DATA &data);

    bool            writeHeader();
    bool            writeHeaderElemCntDetails(const PWGM_ELEMCOUNTS &details);
    bool            writeNodes();
    bool            writeElements();
    bool            writeTopoRegionSets();
    bool            writeListState();
    bool            writeFooter();
    const char *    makeTRSName(const PWGM_CONDDATA &cond) const;

    template<typename T>
    inline bool
    writeCmd(const char *cmd, const T v)
    {
        return rtFile_.write(cmd) && rtFile_.write(v, "\n", " ");
    }

    template<typename T1, typename T2>
    inline bool
    writeCmd(const char *cmd, const T1 v1, const T2 v2)
    {
        return rtFile_.write(cmd) && rtFile_.write(v1, " ", " ") &&
            rtFile_.write(v2, "\n");
    }

    inline bool
    writeCmd(const char *cmd, const char *val)
    {
        return rtFile_.write(cmd) && rtFile_.write(" ") &&
            rtFile_.write(val) && rtFile_.write("\n");
    }

    inline bool
    writeXY(const PWP_REAL x, const PWP_REAL y)
    {
        return rtFile_.write(x, " ") && rtFile_.write(y, "\n");
    }

    inline bool
    writeXYZ(const PWP_REAL x, const PWP_REAL y, const PWP_REAL z)
    {
        return rtFile_.write(x, " ") && rtFile_.write(y, " ") &&
            rtFile_.write(z, "\n");
    }

    inline bool
    writeIndexList(const CaeUnsElementData &ed, const char *eol = " ")
    {
        const PWP_UINT32 vertCnt = ed.vertCount();
        bool ret = true;
        for (PWP_UINT32 ii = 0; ii < vertCnt - 1; ++ii) {
            if (!rtFile_.write(ed.indexAt(ii), " ")) {
                ret = false;
                break;
            }
        }
        if (!rtFile_.write(ed.indexAt(vertCnt - 1), eol)) {
            ret = false;
        }
        return ret;
    }

private:
    bool        isFV_;      //! true/false if FiniteVolume/FiniteElement
    PWP_UINT    numEq_;     //! Number of equations per solution element
    PWP_UINT32  patchId_;   //! Transient value used by face streaming
};

#endif // _CAEUNSCOOLFLUID_H_
