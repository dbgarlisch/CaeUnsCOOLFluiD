# Pointwise V18.1R2 Journal file - Sun Aug 26 03:33:15 2018

package require PWI_Glyph 2.18.1

namespace eval App {
    variable scriptDir      [file dirname [info script]]
    variable baseName       "TrivialMesh"
    variable defaultPwFile  [file join $scriptDir "${baseName}.pw"]

  proc run {} {
    variable scriptDir
    variable baseName

    set blks [getBlocks]

    foreach attr {FiniteVolume FiniteElement} {
      pw::Application setCAESolverAttribute {CalculationType} $attr
      pw::Application markUndoLevel {Set Solver Attributes}
      set ve [string range $attr 6 6]  ;# extract the 'V' or 'E' from $attr
      foreach fmt {ASCII Binary} {
        set deco [string toupper "F${ve}M_${fmt}"]
        set outName "${baseName}_${deco}_Pointwise.CFmesh"
        puts "Exporting: '$outName'"
        set ioCAE [pw::Application begin CaeExport $blks]
          $ioCAE initialize -strict -type CAE [file join $scriptDir $outName]
          $ioCAE setAttribute FileFormat $fmt
          $ioCAE verify
          $ioCAE write
        $ioCAE end
        unset ioCAE
      }
    }
  }

  proc getBlocks {} {
    variable defaultPwFile
    set blks [pw::Entity sort [pw::Grid getAll -type pw::Block]]
    if { 0 != [llength $blks] } {
      puts "INFO: Exporting blocks already loaded."
      variable baseName
      set baseName "Preloaded"
    } elseif { ![file exists $defaultPwFile] } {
      return -code error "Default pw file does not exist: '$defaultPwFile'"
    } elseif { [catch {loadPwFile $defaultPwFile} err] } {
      return -code error "Could not load default pw file: '$defaultPwFile'\nERROR: '$err'"
    } else {
      puts "INFO: Loaded default pw file: '$defaultPwFile'"
      set blks [pw::Entity sort [pw::Grid getAll -type pw::Block]]
    }
    if { 0 == [llength $blks] } {
      return -code error "No blocks to export"
    }

    #set blkNames [list]
    #foreach blk $blks {
    #  lappend blkNames [list [$blk getName]]
    #}
    #puts "Found [llength $blks] blocks: [join $blkNames { }]"

    return $blks
  }

  proc loadPwFile { pwFile } {
    pw::Application reset
    set ioPW [pw::Application begin ProjectLoader]
      $ioPW initialize $pwFile
      $ioPW setAppendMode false
      $ioPW setRepairMode Defer
      $ioPW load
    $ioPW end
    unset ioPW
    return 1
  }
}

App::run
