#!/bin/sh
#===============================================================================
# Copyright 2003-2017 Intel Corporation All Rights Reserved.
#
# The source code,  information  and material  ("Material") contained  herein is
# owned by Intel Corporation or its  suppliers or licensors,  and  title to such
# Material remains with Intel  Corporation or its  suppliers or  licensors.  The
# Material  contains  proprietary  information  of  Intel or  its suppliers  and
# licensors.  The Material is protected by  worldwide copyright  laws and treaty
# provisions.  No part  of  the  Material   may  be  used,  copied,  reproduced,
# modified, published,  uploaded, posted, transmitted,  distributed or disclosed
# in any way without Intel's prior express written permission.  No license under
# any patent,  copyright or other  intellectual property rights  in the Material
# is granted to  or  conferred  upon  you,  either   expressly,  by implication,
# inducement,  estoppel  or  otherwise.  Any  license   under such  intellectual
# property rights must be express and approved by Intel in writing.
#
# Unless otherwise agreed by Intel in writing,  you may not remove or alter this
# notice or  any  other  notice   embedded  in  Materials  by  Intel  or Intel's
# suppliers or licensors in any way.
#===============================================================================

mkl_help() {
    echo ""
    echo "Syntax:"
    echo "  $SCRIPT_NAME <arch> [MKL_interface] [${MOD_NAME}]"
    echo ""
    echo "   <arch> must be one of the following"
    echo "       ia32         : Setup for IA-32 architecture"
    echo "       intel64      : Setup for Intel(R) 64 architecture"
    echo ""
    echo "   ${MOD_NAME} (optional) - set path to Intel(R) MKL F95 modules"
    echo ""
    echo "   MKL_interface (optional) - Intel(R) MKL programming interface for intel64"
    echo "                              Not applicable without ${MOD_NAME}"
    echo "       lp64         : 4 bytes integer (default)"
    echo "       ilp64        : 8 bytes integer"
    echo ""
    echo "If the arguments to the sourced script are ignored (consult docs for"
    echo "your shell) the alternative way to specify target is environment"
    echo "variables COMPILERVARS_ARCHITECTURE or MKLVARS_ARCHITECTURE to pass"
    echo "<arch> to the script, MKLVARS_INTERFACE to pass <MKL_interface> and"
    echo "MKLVARS_MOD to pass <MKL_MOD_NAME>"
    echo ""
}

set_mkl_env() {
    CPRO_PATH="/opt/intel/compilers_and_libraries_2017.4.181/mac"
    CPRO_PATH=$2
	echo "CPRO PATH IS" ${CPRO_PATH}
    export MKLROOT=${CPRO_PATH}
	echo ${MKLROOT}

    local SCRIPT_NAME=$0
    local MOD_NAME=mod

    local MKL_LP64_ILP64=
    local MKL_MOD=
    local MKL_TARGET_ARCH=
    local MKL_TARGET_ARCH_SUBDIR
    local MKLVARS_VERBOSE=
    local MKL_BAD_SWITCH=
    local OLD_DYLD_LIBRARY_PATH=
    local OLD_LIBRARY_PATH=
    local OLD_NLSPATH=
    local OLD_CPATH=

    if  [ -z "$1" ] ; then
      if [ -n "$MKLVARS_ARCHITECTURE" ] ; then
        MKL_TARGET_ARCH="$MKLVARS_ARCHITECTURE"
      elif [ -n "$COMPILERVARS_ARCHITECTURE" ] ; then
        MKL_TARGET_ARCH="$COMPILERVARS_ARCHITECTURE"
      fi
      if [ "${MKL_TARGET_ARCH}" != "ia32" -a "${MKL_TARGET_ARCH}" != "intel64" ] ; then
        MKL_TARGET_ARCH=
      fi
      if [ -n "$MKLVARS_INTERFACE" ] ; then
        MKL_LP64_ILP64="$MKLVARS_INTERFACE"
        if [ "${MKL_LP64_ILP64}" != "lp64" -a "${MKL_LP64_ILP64}" != "ilp64" ] ; then
          MKL_LP64_ILP64=
        fi
      fi
      if [ -n "$MKLVARS_MOD" ] ; then
        MKL_MOD="$MKLVARS_MOD"
      fi
      if [ -n "$MKLVARS_VERBOSE" ] ; then
        MKLVARS_VERBOSE="$MKLVARS_VERBOSE"
      fi
    else
        if [ -n "$1" ]; then
           if   [ "$1" = "ia32" ]        ; then MKL_TARGET_ARCH=ia32; MKL_TARGET_ARCH_SUBDIR=ia32_mac;
           elif [ "$1" = "intel64" ]     ; then MKL_TARGET_ARCH=intel64; MKL_TARGET_ARCH_SUBDIR=intel64_mac;
           elif [ "$1" = "lp64" ]        ; then MKL_LP64_ILP64=lp64;
           elif [ "$1" = "ilp64" ]       ; then MKL_LP64_ILP64=ilp64;
           elif [ "$1" = "${MOD_NAME}" ] ; then MKL_MOD=${MOD_NAME};
           elif [ "$1" = "verbose" ]     ; then MKLVARS_VERBOSE=verbose;
           else
               MKL_BAD_SWITCH=$1
               break 10
           fi
           #shift;
	   fi
    fi

    if [ -n "${MKL_BAD_SWITCH}" ] ; then

      echo
      echo "ERROR: Unknown option '${MKL_BAD_SWITCH}'"
      mkl_help

    else

        if [ -z "${MKL_TARGET_ARCH}" ] ; then

            echo
            echo "ERROR: architecture is not defined. Accepted values: ia32, intel64"
            mkl_help

        else
            typeset mkl_ld_arch="${CPRO_PATH}/compiler/lib:${MKLROOT}/lib"

            if [ -z "${TBBROOT}" ]; then
                __tbb_path="${CPRO_PATH}/tbb/lib"
                if [ -d "${__tbb_path}" ]; then
                    mkl_ld_arch="${__tbb_path}:${mkl_ld_arch}"
                fi
            fi

            if [ -n "${DYLD_LIBRARY_PATH}" ]; then OLD_DYLD_LIBRARY_PATH=":${DYLD_LIBRARY_PATH}"; fi
            export DYLD_LIBRARY_PATH="${mkl_ld_arch}${OLD_DYLD_LIBRARY_PATH}"

            if [ -n "${LIBRARY_PATH}" ]; then OLD_LIBRARY_PATH=":${LIBRARY_PATH}"; fi
            export LIBRARY_PATH="${mkl_ld_arch}${OLD_LIBRARY_PATH}"

            if [ -n "${NLSPATH}" ]; then OLD_NLSPATH=":${NLSPATH}"; fi
            export NLSPATH="${MKLROOT}/lib/locale/%l_%t/%N${OLD_NLSPATH}"

            if [ -n "$CPATH" ]; then OLD_CPATH=":${CPATH}"; fi
            export CPATH="${MKLROOT}/include${OLD_CPATH}"

            if [ "${MKL_MOD}" = "${MOD_NAME}" ] ; then
                if [ "${MKL_TARGET_ARCH}" = "ia32" ] ; then
                    MKL_LP64_ILP64=
                else
                    if [ -z "$MKL_LP64_ILP64" ] ; then
                        MKL_LP64_ILP64=lp64
                    fi
                fi
                export CPATH="${CPATH}:${MKLROOT}/include/${MKL_TARGET_ARCH_SUBDIR}/${MKL_LP64_ILP64}"
            fi

            if [ "${MKLVARS_VERBOSE}" = "verbose" ] ; then
                echo DYLD_LIBRARY_PATH=${DYLD_LIBRARY_PATH}
                echo LIBRARY_PATH=${LIBRARY_PATH}
                echo NLSPATH=${NLSPATH}
                echo CPATH=${CPATH}
            fi
        fi
    fi
}

set_mkl_env "$@"

