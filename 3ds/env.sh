# Source this to get the devkitPro environment for a user-local install:
#   . ./env.sh
export DEVKITPRO="${DEVKITPRO:-$HOME/devkitpro}"
export DEVKITARM="$DEVKITPRO/devkitARM"
export DEVKITPRO_PKG="$DEVKITPRO/portlibs/3ds"
export PATH="$DEVKITPRO/tools/bin:$DEVKITARM/bin:$PATH:$DEVKITPRO_PKG/bin"
