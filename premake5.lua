workspace "Truetype"
    architecture "x64"

    configurations { 
        "Debug", 
        "Release",
        "Dist"
    }

    startproject "TruetypeExamples"

-- This is a helper variable, to concatenate the sys-arch
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "Truetype"
include "TruetypeExamples"
include "TruetypeTests"
