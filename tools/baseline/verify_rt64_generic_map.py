"""Verify the baseline S20 fixture resolves stock GameFrame methods from its RT64 archive."""
import argparse
from pathlib import Path

SYMBOLS = ('?areFramebufferPairsCompatible@GameFrame@RT64@@QEAA_NAEBUWorkloadQueue@2@AEBUFramebufferPair@GameIndices@2@1@Z', '?isSceneCompatible@GameFrame@RT64@@QEAA_NAEBUWorkloadQueue@2@AEBUGameScene@2@AEBUProjection@GameIndices@2@@Z', '?set@GameFrame@RT64@@QEAAXAEAUWorkloadQueue@2@PEBII@Z', '?computeTransformMatch@RT64@@YA?AUTransformMatchResult@1@AEBUfloat4x4@hlslpp@@000PEBURigidBody@1@@Z', '?match@GameFrame@RT64@@QEAAXPEAURenderWorker@2@AEAUWorkloadQueue@2@AEBU12@PEAUBufferUploader@2@AEA_N44@Z', '?buildTransformIdMap@GameFrame@RT64@@QEBAXAEBUWorkload@2@AEAV?$multimap@IIU?$less@I@std@@V?$allocator@U?$pair@$$CBII@std@@@2@@std@@AEAV?$vector@IV?$allocator@I@std@@@5@@Z', '?matchTransform@GameFrame@RT64@@QEAAXAEAUWorkload@2@AEBU32@AEAUWorkloadMap@GameFrameMap@2@PEBU452@IIAEAUModifiedBuffers@2@@Z', '?matchScene@GameFrame@RT64@@QEAAXAEAUWorkloadQueue@2@AEBU12@AEBUGameScene@2@2AEAV?$unordered_map@IUModifiedBuffers@RT64@@U?$hash@I@std@@U?$equal_to@I@4@V?$allocator@U?$pair@$$CBIUModifiedBuffers@RT64@@@std@@@4@@std@@AEA_N4@Z', '?buildCallHashMap@GameFrame@RT64@@QEBAXIAEBUWorkload@2@AEBUProjection@2@AEAV?$multimap@_KUGameCallMap@RT64@@U?$less@_K@std@@V?$allocator@U?$pair@$$CB_KUGameCallMap@RT64@@@std@@@4@@std@@@Z', '?hashFromCall@GameFrame@RT64@@QEBA_KAEBUGameCall@2@I@Z', '?isDebuggerCameraEnabled@GameFrame@RT64@@QEAA_NAEBUWorkloadQueue@2@@Z')


def verify(path):
    lines = path.read_text().splitlines()
    for symbol in SYMBOLS:
        rows = [line for line in lines if " " + symbol + " " in line]
        if len(rows) != 1 or rows[0].split()[-1] != "rt64:rt64_game_frame.cpp.obj":
            raise ValueError("Expected sole baseline RT64 archive owner: " + symbol + repr(rows))
    if any("tooieRepairSourceMatrixAliases" in line for line in lines):
        raise ValueError("Product alias-repair helper must be absent from baseline owner fixture")
    return len(SYMBOLS)


POP_SYMBOL_OWNERS = {
    '?extendedOp@GBI_EXTENDED@RT64@@YAXPEAUState@2@PEAPEAUDisplayList@2@@Z': 'rt64:rt64_gbi_extended.cpp.obj',
    '?popMatrixId@RSP@RT64@@QEAAXE_N@Z': 'rt64:rt64_rsp.cpp.obj',
}


def verify_projection_pop(path):
    lines = path.read_text().splitlines()
    for symbol, owner in POP_SYMBOL_OWNERS.items():
        rows = [line for line in lines if " " + symbol + " " in line]
        if len(rows) != 1 or rows[0].split()[-1] != owner:
            raise ValueError("Expected sole projection POP archive owner: " + symbol + repr(rows))
    return len(POP_SYMBOL_OWNERS)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--map", required=True, type=Path)
    parser.add_argument("--projection-pop", action="store_true")
    args = parser.parse_args()
    if args.projection_pop:
        print("baseline RT64 projection POP map: sole_archive_origins=" + str(verify_projection_pop(args.map)))
    else:
        print("baseline RT64 owner map: sole_archive_origins=" + str(verify(args.map)) + " product_alias_helper=absent")
