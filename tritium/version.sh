#!/bin/sh
[ -n "$2" ] && cd $2

MAJOR=0
MINOR=10
# Set below blank for the commit to be tagged with the above version.
SUFFIX="-post"
BUILD=0

git_describe() {
    # More detail than git describe --tagso
    TAG=`git tag -l v$MAJOR.$MINOR`
    if [ `git rev-list origin -n 1 2>/dev/null | wc -l` -eq 0 ]
    then ORIGIN=HEAD
    else ORIGIN=origin
    fi

    SUFFIX=
    VER=`git rev-list HEAD ${TAG:+^$TAG} | wc -l`
    OVER=`git rev-list $ORIGIN ${TAG:+^$TAG} | wc -l`
    DIFF=`git rev-list $ORIGIN..HEAD | wc -l`
    NDIFF=`git rev-list HEAD..$ORIGIN | wc -l`

    MOD=`git status --porc | grep "^ M" | wc -l`

    HDR=`( git rev-list HEAD -n 1 ; git rev-list $ORIGIN -n 1 ) 2>/dev/null | uniq | wc -l`

    [ "$DIFF" -eq "0" -a "$NDIFF" -eq 0 -a "$HDR" -eq 2 ] && {
	echo WARNING: YOUR VERSION IS FORKED 1>&2
	SUFFIX="-FORK"
    }

    VER=$((VER-DIFF))

    echo Origin ver: $MAJOR.$MINOR.$VER, Local commits: $DIFF, Local edits: $MOD 1>&2
    [ "$OVER" -gt "$VER" ] &&
	echo WARNING: Origin is ahead of your current HEAD by $((OVER-VER)) commits. 1>&2

    [ "$DIFF" -ne 0 ] && SUFFIX="$SUFFIX+$DIFF"
    [ "$MOD" -ne 0 -a "$DIFF" -eq 0 ] && SUFFIX="${SUFFIX}+0"
    [ "$MOD" -ne 0 ] && SUFFIX="${SUFFIX}.${MOD}"

    [ "$TAG" = "" ] && SUFFIX="$SUFFIX-pre"

    SUFFIX="$SUFFIX `git rev-list HEAD -n 1 | cut -c 1-7`"
    BUILD="$VER"
}

((type git && git status ) >/dev/null 2>&1) && git_describe

if [ -n "$1" ]
then TMPFILE="$1".tmp
else TMPFILE=/dev/stdout
fi

{
echo "#define VERSION_MAJOR    $MAJOR"
echo "#define VERSION_MINOR    $MINOR"
echo "#define VERSION_BUILD    $BUILD"
echo "#define VERSION_SUFFIX   \"$SUFFIX\""
} > "$TMPFILE"

[ -n "$1" ] && {
    if ! cmp -s "$TMPFILE" "$1"
    then
	mv "$TMPFILE" "$1"
    else rm -f "$TMPFILE"
    fi
}

exit 0