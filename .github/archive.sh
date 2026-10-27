#!/bin/bash
# Creates source archive. Untracked files are ignored.
#
# Usage: .github/archive.sh [output-dir]
set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
OUTPUT_DIR="$(cd "${1:-.}" && pwd)"

if [ ! -e "$ROOT_DIR/core-lib/README.md" ]; then
  echo "core-lib is not checked out, run: git submodule update --init" >&2
  exit 1
fi

VERSION="$(git -C "$ROOT_DIR" describe --tags --always --dirty)"
NAME="SOMpp-$VERSION"
ARCHIVE="$OUTPUT_DIR/$NAME.tar.gz"

STAGING_DIR="$(mktemp -d)"
trap 'rm -rf "$STAGING_DIR"' EXIT
mkdir "$STAGING_DIR/$NAME"

(cd "$ROOT_DIR" && git ls-files -z --recurse-submodules | tar -cf - --null -T -) \
  | tar -xf - -C "$STAGING_DIR/$NAME"

tar -cf - -C "$STAGING_DIR" "$NAME" | gzip -9 > "$ARCHIVE"

echo "Created $ARCHIVE"

ZIP="${ARCHIVE%.tar.gz}.zip"
(cd "$STAGING_DIR" && zip -9 -r -y -q "$ZIP" "$NAME")

echo "Created $ZIP"

echo "ARCHIVE=$ARCHIVE" >> "$GITHUB_ENV"
echo "ZIP=$ZIP" >> "$GITHUB_ENV"
