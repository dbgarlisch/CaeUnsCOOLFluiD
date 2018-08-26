# Pointwise V18.1R2 Journal file - Sun Aug 26 03:33:15 2018

package require PWI_Glyph 2.18.1

set scriptDir [file dirname [info script]]

pw::Application setUndoMaximumLevels 5
pw::Application reset
pw::Application markUndoLevel {Journal Reset}

pw::Application clearModified

pw::Application reset -keep Clipboard
set ioPW [pw::Application begin ProjectLoader]
  $ioPW initialize [file join $scriptDir TrivialMesh.pw]
  $ioPW setAppendMode false
  $ioPW setRepairMode Defer
  $ioPW load
$ioPW end
unset ioPW
pw::Application resetUndoLevels

set blks [pw::Entity sort [pw::Grid getAll -type pw::Block]]
puts "blks=[list $blks]"

pw::Application setCAESolverAttribute {CalculationType} {FiniteVolume}
pw::Application markUndoLevel {Set Solver Attributes}


set ioCAE [pw::Application begin CaeExport $blks]
  $ioCAE initialize -strict -type CAE [file join $scriptDir junk-ASCII-FVM.CFmesh]
  $ioCAE setAttribute FileFormat ASCII
  $ioCAE verify
  $ioCAE write
$ioCAE end
unset ioCAE


set ioCAE [pw::Application begin CaeExport $blks]
  $ioCAE initialize -strict -type CAE [file join $scriptDir junk-BINARY-FVM.CFmesh]
  $ioCAE setAttribute FileFormat Binary
  $ioCAE verify
  $ioCAE write
$ioCAE end
unset ioCAE


pw::Application setCAESolverAttribute {CalculationType} {FiniteElement}
pw::Application markUndoLevel {Set Solver Attributes}

set ioCAE [pw::Application begin CaeExport $blks]
  $ioCAE initialize -strict -type CAE [file join $scriptDir junk-ASCII-FEM.CFmesh]
  $ioCAE setAttribute FileFormat ASCII
  $ioCAE verify
  $ioCAE write
$ioCAE end
unset ioCAE


set ioCAE [pw::Application begin CaeExport $blks]
  $ioCAE initialize -strict -type CAE [file join $scriptDir junk-BINARY-FEM.CFmesh]
  $ioCAE setAttribute FileFormat Binary
  $ioCAE verify
  $ioCAE write
$ioCAE end
unset ioCAE
