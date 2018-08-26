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
    bool            writeElement(const PWGM_ELEMDATA &ed, const PWP_UINT32 eNdx);
    bool            writeTopoRegionSets();
    bool            writeListState();
    bool            writeFooter();
    const char *    makeTRSName(const PWGM_CONDDATA &cond) const;

    template<typename T>
    inline bool
    writeCmd(const char *cmd, const T v)
    {
        const PWP_INT wd = (isAsciiEncoding() ? -1 : 30);
        return rtFile_.write(cmd, wd, ' ') && rtFile_.write(v, 0, " ") &&
            rtFile_.write("\n");
    }

    template<typename T1, typename T2>
    inline bool
    writeCmd(const char *cmd, const T1 v1, const T2 v2)
    {
        const PWP_INT wd = (isAsciiEncoding() ? -1 : 30);
        return rtFile_.write(cmd, wd, ' ') && rtFile_.write(v1, " ", " ") &&
            rtFile_.write(v2) &&
            rtFile_.write("\n");
    }

    template<typename T>
    inline bool
    writeCmd(const char *cmd, const std::vector<T> &vec)
    {
        const PWP_INT wd = (isAsciiEncoding() ? -1 : 30);
        bool ret = rtFile_.write(cmd, wd, ' ');
        if (ret) {
            for (auto&& v : vec) {
                if (!rtFile_.write(v, 0, " ")) {
                    ret = false;
                    break;
                }
            }
        }
        return ret && rtFile_.write("\n");
    }

    template<>
    inline bool
    writeCmd(const char *cmd, const std::vector<std::string> &vec)
    {
        const PWP_INT wd = (isAsciiEncoding() ? -1 : 30);
        bool ret = rtFile_.write(cmd, wd, ' ');
        if (ret) {
            for (auto && v : vec) {
                if (isAsciiEncoding() && !rtFile_.write(" ")) {
                    ret = false;
                    break;
                }
                if (!rtFile_.write(v.c_str(), wd, ' ')) {
                    ret = false;
                    break;
                }
            }
        }
        return ret && rtFile_.write("\n");
    }

    inline bool
    writeCmd(const char *cmd, const char *val)
    {
        const PWP_INT wd = (isAsciiEncoding() ? -1 : 30);
        return rtFile_.write(cmd, wd, ' ') &&
            (isAsciiEncoding() ? rtFile_.write(" ") : true) &&
            rtFile_.write(val, wd, ' ') &&
            rtFile_.write("\n");
    }

    inline bool
    writeCmd(const char *cmd)
    {
        const PWP_INT wd = (isAsciiEncoding() ? -1 : 30);
        return rtFile_.write(cmd, wd, ' ') && rtFile_.write("\n");
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
    writeIndexList(const PWGM_ELEMDATA &ed, const char *eol)
    {
        const PWP_UINT32 vertCnt = ed.vertCnt;
        bool ret = true;
        for (PWP_UINT32 ii = 0; ii < vertCnt - 1; ++ii) {
            if (!rtFile_.write(ed.index[ii], " ")) {
                ret = false;
                break;
            }
        }
        if (ret && !rtFile_.write(ed.index[vertCnt - 1])) {
            ret = false;
        }
        if (ret && (0 != eol) && !rtFile_.write(eol)) {
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
