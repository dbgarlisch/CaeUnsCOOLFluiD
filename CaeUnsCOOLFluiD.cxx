/****************************************************************************
 *
 * class CaeUnsCOOLFluiD
 *
 * Proprietary software product of Pointwise, Inc.
 * Copyright (c) 1995-2016 Pointwise, Inc.
 * All rights reserved.
 *
 ***************************************************************************/

#include <cassert>
#include <sstream>

#include "apiCAEP.h"
#include "apiCAEPUtils.h"
#include "apiGridModel.h"
#include "apiPWP.h"
#include "runtimeWrite.h"
#include "pwpPlatform.h"

#include "CaePlugin.h"
#include "CaeUnsGridModel.h"
#include "CaeUnsCOOLFluiD.h"

/*
From:
https://github.com/andrealani/COOLFluiD/wiki/CFmesh-format-for-parallel-I-O

!COOLFLUID_VERSION 2013.9
!CFMESH_FORMAT_VERSION 1.3
!NB_DIM 3
!NB_EQ 9
!NB_NODES 36972 0
!NB_STATES 197060 0
!NB_ELEM 197060
!NB_ELEM_TYPES 1
!GEOM_POLYORDER 1
!SOL_POLYORDER 0
!ELEM_TYPES Tetra
!NB_ELEM_PER_TYPE 197060
!NB_NODES_PER_TYPE 4
!NB_STATES_PER_TYPE 1
!LIST_ELEM
499 5938 5939 4565 0
...snip...
33581 36797 33088 31865 197059
!NB_TRSs 3
!TRS_NAME SuperInlet
!NB_TRs 1
!NB_GEOM_ENTS 1768
!GEOM_TYPE Face
!LIST_GEOM_ENT
3 1 937 5008 2 589
...snip...
3 1 5783 5776 5782 155308
!TRS_NAME SlipWall
!NB_TRs 1
!NB_GEOM_ENTS 304
!GEOM_TYPE Face
!LIST_GEOM_ENT
3 1 56 1 28 396
...snip...
3 1 113 104 107 56472
!TRS_NAME SuperOutlet
!NB_TRs 1
!NB_GEOM_ENTS 13904
!GEOM_TYPE Face
!LIST_GEOM_ENT
3 1 9 9055 989 147967
...snip...
3 1 10604 10533 10605 90759
!LIST_NODE
-2.50000000000000e+00 0.00000000000000e+00 0.00000000000000e+00
...snip...
1.49843183055000e+02 1.39931891975000e+01 2.63177682383000e+01
!LIST_STATE 0
!END
*/

//***************************************************************************
//***************************************************************************
//***************************************************************************

CaeUnsCOOLFluiD::CaeUnsCOOLFluiD(CAEP_RTITEM *pRti, PWGM_HGRIDMODEL
        model, const CAEP_WRITEINFO *pWriteInfo) :
    CaeUnsPlugin(pRti, model, pWriteInfo),
    isFV_(true),
    numEq_(1),
    patchId_(PWP_UINT32_MAX)
{
}

CaeUnsCOOLFluiD::~CaeUnsCOOLFluiD()
{
}

bool
CaeUnsCOOLFluiD::beginExport()
{
    PWP_UINT calcType = FiniteVolume;
    if (model_.getAttribute("CalculationType", calcType)) {
        switch (calcType) {
        case FiniteVolume:
            isFV_ = true;
            break;
        case FiniteElement:
            isFV_ = false;
            break;
        default:
            // should never get here
            ASSERT(0 == (void*)"Bad CalculationType in beginExport()");
            break;
        }
    }

    model_.getAttribute("NumEquations", numEq_);

    bool ret = true;
    setProgressMajorSteps(3); // Elements, TopoRegionSets Nodes
    if (isDimension3D()) {
        ret = model_.appendEnumElementOrder(PWGM_ELEMORDER_HEX) &&
            model_.appendEnumElementOrder(PWGM_ELEMORDER_TET) &&
            model_.appendEnumElementOrder(PWGM_ELEMORDER_WEDGE) &&
            model_.appendEnumElementOrder(PWGM_ELEMORDER_PYRAMID);
    }
    else {
        ret = model_.appendEnumElementOrder(PWGM_ELEMORDER_QUAD) &&
            model_.appendEnumElementOrder(PWGM_ELEMORDER_TRI);
    }
    return ret;
}

PWP_BOOL
CaeUnsCOOLFluiD::write()
{
    return (writeHeader() && writeElements() && writeTopoRegionSets() &&
        writeNodes() && writeListState() && writeFooter()) ? PWP_TRUE :
        ((rti_.opAborted = true), PWP_FALSE);
}

bool
CaeUnsCOOLFluiD::endExport()
{
    return true;
}

bool
CaeUnsCOOLFluiD::writeHeader()
{
    //!COOLFLUID_VERSION 2013.9
    //!CFMESH_FORMAT_VERSION 1.3
    //!NB_DIM 3
    //!NB_EQ 1
    //!NB_NODES #nodes 0
    //!NB_STATES #states 0
    //!NB_ELEM #totNumElems
    //!NB_ELEM_TYPES #numElemTypes
    //!GEOM_POLYORDER 1
    //!SOL_POLYORDER 0
    //!ELEM_TYPES #listOf(Line|Triag|Quad|Tetra|Pyram|Prism|Hexa)
    //!NB_ELEM_PER_TYPE #listOf(numLines|numTriag|numQuad|numTetra|numPyram|numPrism|numHexa)
    //!NB_NODES_PER_TYPE #listOf(2|3|4|4|5|6|8)  //for linear mesh
    //!NB_STATES_PER_TYPE #listOf(nbLinesS|nbTriagS|nbQuadS|nbTetraS|nbPyramS|nbPrismS|nbHexaS)

    //For FVM calculations we need:  
    //    !NB_STATES == !NB_ELEMS
    //    !SOL_POLYORDER 0
    //    For TRS: faces are defined with vertices and a single internal state  
    //    !LIST_GEOM_ENT
    //    #number_of_face_vertices 1 937 5008 2 589
    //
    //For a FEM (P1-Q1):
    //    !NB_STATES == !NB_NODES
    //    !SOL_POLYORDER 1
    //    For TRS: faces are defined with vertices and the same number of states
    //    !LIST_GEOM_ENT
    //    #number_of_face_vertices #number_of_face_vertices 937 5008 2 589

    PWGM_ELEMCOUNTS ecDetails = { {0} };
    const PWP_UINT Zero = 0;
    const PWP_UINT32 totElemCnt = model_.elementCount(&ecDetails);
    const PWP_UINT32 numVerts = model_.vertexCount();
    const PWP_UINT32 numStates = (isFV_ ? totElemCnt : numVerts);
    const PWP_UINT32 gPolyOrder = (isFV_ ? 1 : 1);
    const PWP_UINT32 sPolyOrder = (isFV_ ? 0 : 1);
    const PWP_UINT32 dim = (isDimension3D() ? 3 : 2);
    return rtFile_.isOpen() &&
        writeCmd("!COOLFLUID_VERSION", "2013.9") &&
        writeCmd("!CFMESH_FORMAT_VERSION", "1.3") &&
        writeCmd("!NB_DIM", dim) &&
        writeCmd("!NB_EQ", numEq_) &&
        writeCmd("!NB_NODES", numVerts, Zero) &&
        writeCmd("!NB_STATES", numStates, Zero) &&
        writeCmd("!NB_ELEM", totElemCnt) &&
        writeCmd("!GEOM_POLYORDER", gPolyOrder) &&
        writeCmd("!SOL_POLYORDER", sPolyOrder) &&
        writeHeaderElemCntDetails(ecDetails);
}

bool
CaeUnsCOOLFluiD::writeHeaderElemCntDetails(const PWGM_ELEMCOUNTS &details)
{
    //!NB_ELEM_TYPES #numElemTypes
    //!ELEM_TYPES #listOf(Line|Triag|Quad|Tetra|Pyram|Prism|Hexa)
    //!NB_ELEM_PER_TYPE #listOf(nbLines|nbTriag|nbQuad|nbTetra|nbPyram|nbPrism|nbHexa)
    //!NB_NODES_PER_TYPE #listOf(2|3|4|4|5|6|8)  //for linear mesh
    //!NB_STATES_PER_TYPE #listOf(nbLinesS|nbTriagS|nbQuadS|nbTetraS|nbPyramS|nbPrismS|nbHexaS)

    PWP_UINT32 nbElemTypes = 0;
    std::vector<std::string> elemTypes;
    std::vector<PWP_UINT32> nbElemPerType;
    std::vector<PWP_UINT32> nbNodesPerType;
    std::vector<PWP_UINT32> nbStatesPerType;
    auto f = [&nbElemTypes, &elemTypes, &nbElemPerType, &nbNodesPerType,
                &nbStatesPerType](const PWP_UINT32 cnt, const char *type,
                    const PWP_UINT32 numNodes, const bool isFiniteVol)
                {
                    if (0 != cnt) {
                        ++nbElemTypes;
                        elemTypes.push_back(type);
                        nbElemPerType.push_back(cnt);
                        nbNodesPerType.push_back(numNodes);
                        nbStatesPerType.push_back(isFiniteVol ? 1 : numNodes);
                    }
                };
    if (isDimension3D()) {
        f(PWGM_ECNT_Hex(details), "Hexa", 8, isFV_);
        f(PWGM_ECNT_Tet(details), "tetra", 4, isFV_);
        f(PWGM_ECNT_Wedge(details), "Prism", 6, isFV_);
        f(PWGM_ECNT_Pyramid(details), "Pyram", 5, isFV_);
    }
    else {
        f(PWGM_ECNT_Quad(details), "Quad", 4, isFV_);
        f(PWGM_ECNT_Tri(details), "Triag", 3, isFV_);
    }

    return (0 < nbElemTypes) &&
        writeCmd("!NB_ELEM_TYPES", nbElemTypes) &&
        writeCmd("!ELEM_TYPES", elemTypes) &&
        writeCmd("!NB_ELEM_PER_TYPE", nbElemPerType) &&
        writeCmd("!NB_NODES_PER_TYPE", nbNodesPerType) &&
        writeCmd("!NB_STATES_PER_TYPE", nbStatesPerType);
}

bool
CaeUnsCOOLFluiD::writeNodes()
{
    //!LIST_NODE
    //x y [z]   (repeat NB_NODES times)
    bool ret = progressBeginStep(model_.vertexCount()) &&
        writeCmd("!LIST_NODE");
    if (ret) {
        CaeUnsVertex vert(model_);
        while (vert.isValid()) {
            if (isDimension3D()) {
                if (!writeXYZ(vert.x(), vert.y(), vert.z())) {
                    ret = false;
                    break;
                }
            }
            else {
                if (!writeXY(vert.x(), vert.y())) {
                    ret = false;
                    break;
                }
            }
            if (!progressIncrement()) {
                ret = false;
                break;
            }
            ++vert;
        }
    }
    return progressEndStep() && ret;
}

bool
CaeUnsCOOLFluiD::writeElements()
{
    //!LIST_ELEM
    // NodeNdxList StateList   (repeat NB_ELEM times)
    bool ret = progressBeginStep(model_.elementCount()) &&
        writeCmd("!LIST_ELEM");
    if (ret) {
        CaeUnsElementData ed;
        CaeUnsElement elem(model_);
        while (elem.isValid()) {
            if (!elem.data(ed) || !writeElement(ed, elem.index()) ||
                !progressIncrement()) {
                ret = false;
                break;
            }
            ++elem;
        }
        if (ret && isBinaryEncoding() && !rtFile_.write("\n")) {
            ret = false;
        }
    }
    return progressEndStep() && ret;
}

bool
CaeUnsCOOLFluiD::writeElement(const PWGM_ELEMDATA &ed, const PWP_UINT32 eNdx)
{
    // NodeNdxList StateList
    const bool isAscii = isAsciiEncoding();
    // write NodeNdxList
    bool ret = writeIndexList(ed, (isAscii ? " " : 0));
    if (ret) {
        // write StateList
        if (isFV_) {
            // One state value per volume elem (= global cell index)
            const PWGM_ELEMDATA edFV{ PWGM_ELEMTYPE_SIZE, 1, {}, {eNdx} };
            if (!writeIndexList(edFV, (isAscii ? "\n" : 0))) {
                ret = false;
            }
        }
        else {
            // One state value per volume elem node (= global vert index)
            if (!writeIndexList(ed, (isAscii ? "\n" : 0))) {
                ret = false;
            }
        }
    }
    return ret;
}

bool
CaeUnsCOOLFluiD::writeTopoRegionSets()
{
    //!NB_TRSs 3
    //---foreach TR (aka BC):
    return writeCmd("!NB_TRSs", model_.patchCount()) &&
        model_.streamFaces(PWGM_FACEORDER_BCGROUPSONLY, *this);
}

bool
CaeUnsCOOLFluiD::writeListState()
{
    //!LIST_STATE 0
    // or
    //!LIST_STATE 1
    //EqVal_1 ... EqVal_NB_EQ (repeated !NB_STATES)
    return writeCmd("!LIST_STATE", 0);
}

bool
CaeUnsCOOLFluiD::writeFooter()
{
    return rtFile_.write("!END\n");
}


const char *
CaeUnsCOOLFluiD::makeTRSName(const PWGM_CONDDATA &cond) const
{
    static std::string ret;
    ret = cond.name;
    ret += '_';
    ret += cond.type;
    // return ptr only valid until next call
    return ret.c_str();
}



//===========================================================================
// face streaming handlers
//===========================================================================

PWP_UINT32
CaeUnsCOOLFluiD::streamBegin(const PWGM_BEGINSTREAM_DATA &data)
{
    patchId_ = PWP_UINT32_MAX;
    return PWP_CAST_BOOL(progressBeginStep(data.totalNumFaces));
}


PWP_UINT32
CaeUnsCOOLFluiD::streamFace(const PWGM_FACESTREAM_DATA &data)
{
    bool ret = true;
    if (patchId_ != PWGM_HDOMAIN_ID(data.owner.domain)) {
        //----- Start a new TR patch
        //!TRS_NAME TRName
        //!NB_TRs 1
        //!NB_GEOM_ENTS NumTRElems
        //!GEOM_TYPE (Face | ???)
        //!LIST_GEOM_ENT
        patchId_ = PWGM_HDOMAIN_ID(data.owner.domain);
        CaeUnsPatch patch(data.owner.domain);
        PWGM_CONDDATA cond;
        ret = patch.condition(cond) &&
            writeCmd("!TRS_NAME", makeTRSName(cond)) &&
            writeCmd("!NB_TRs", (PWP_UINT)1) &&
            writeCmd("!NB_GEOM_ENTS", patch.elementCount()) &&
            writeCmd("!GEOM_TYPE", "Face") &&
            writeCmd("!LIST_GEOM_ENT");
    }
    if (ret) {
        //const CaeUnsElementData ed(data.elemData);
        const PWP_UINT32 vertCnt = data.elemData.vertCnt;
        const PWP_UINT32 stateCnt = (isFV_ ? 1 : vertCnt);
        const bool isAscii = isAsciiEncoding();
        // ElemNumNodes ElemNumStates NodeNdxList StateList
        ret = rtFile_.write(vertCnt, " ") && rtFile_.write(stateCnt, " ") &&
            writeElement(data.elemData, data.owner.cellIndex);
    }
    if (!progressIncrement()) {
        ret = false;
    }
    return PWP_CAST_BOOL(ret);
}

PWP_UINT32
CaeUnsCOOLFluiD::streamEnd(const PWGM_ENDSTREAM_DATA &data)
{
    return PWP_CAST_BOOL(progressEndStep() && data.ok);
}


//===========================================================================
// called ONCE when plugin first loaded into memory
//===========================================================================

bool
CaeUnsCOOLFluiD::create(CAEP_RTITEM &rti)
{
    (void)rti.BCCnt; // silence unused arg warning
    bool ret = true;

    //-----------------------------------------------------------------------
    // BYTE ORDERING:
    //   Set the following flags to control the byte ordering options
    //   supported by the solver. If all flags are false, the plugin will use
    //   the platform's native byte ordering. Currently, Pointwise only runs on
    //   little endian, intel platforms. If the solver targeted by this plugin
    //   cannot import little endian files, you must set bigEndian to true and
    //   littleEndian to false.
    //-----------------------------------------------------------------------
    bool bigEndian = false;
    bool littleEndian = false;
    if (ret && (bigEndian || littleEndian)) {
        ret = allowByteOrders(rti, bigEndian, littleEndian);
    }

    //-----------------------------------------------------------------------
    // ELEMENT TOPOLOGY:
    //   Set the following flags to control the element topology options
    //   supported by the solver. If all flags are false, the allowed element
    //   topologies will be inferred from the supported element types. Unless
    //   this plugin has special needs, you should leave these all false.
    //-----------------------------------------------------------------------
    bool structured = false;
    bool unstructured = false;
    bool prismatic = false;
    if (ret && (structured || unstructured || prismatic)) {
        ret = allowElementTopologies(rti, structured, unstructured, prismatic);
    }

    ret = ret &&
         publishEnumValueDef(rti, "CalculationType", "FiniteVolume",
            "Solution calculation type", "FiniteVolume|FiniteElement") &&
         publishUIntValueDef(rti, "NumEquations", 1,
            "Number of equations per solution element", 1, 50);

    // These attributes are for example only. You can publish any attribute
    // needed for your solver.
    // ret = ret &&
    //     publishUIntValueDef(rti, "iterations", 5, "Number of iterations", 0,
    //          2000) &&
    //     publishIntValueDef(rti, "magnitude", -5, "Signed int magnitude",
    //          -100, 100) &&
    //     publishRealValueDef(rti, "mach", 0.3, "Incoming flow velocity", 0.0,
    //          1000.0, 0.0, 50.0) &&
    //     publishRealValueDef(rti, "temperature", 77.5, "Ambient temperature",
    //          -5000.0, 5000.0, -100.0, 3000.0) &&
    //     publishEnumValueDef(rti, "temperature.units", "Fahrenheit",
    //          "Grid temperature units", "Fahrenheit|Celsius") &&
    //     publishEnumValueDef(rti, "units", "Inches", "Grid dimensional units",
    //          "Yards|Inches|Meters|Millimeters") &&
    //     publishStringValueDef(rti, "description", "", "Grid description") &&
    //     publishBoolValueDef(rti, "linear", false, "Grid is linear",
    //          "reject|accept");

    return ret;
}


//===========================================================================
// called ONCE just before plugin unloaded from memory
//===========================================================================

void
CaeUnsCOOLFluiD::destroy(CAEP_RTITEM &rti)
{
    (void)rti.BCCnt; // silence unused arg warning
}
