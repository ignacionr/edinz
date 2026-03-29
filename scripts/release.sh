#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 <major|minor|patch> [-m <message>] [--push] [--yes]"
    echo ""
    echo "Bumps the semantic version in CMakeLists.txt, regenerates the version"
    echo "header, commits the change, creates a git tag, and optionally pushes."
    echo ""
    echo "Options:"
    echo "  -m <message>   Tag annotation message (default: 'Release vX.Y.Z')"
    echo "  --push         Push the commit and tag to origin after creating them"
    echo "  --yes          Skip confirmation prompts"
    exit 1
}

if [[ $# -lt 1 ]]; then
    usage
fi

BUMP_TYPE="$1"
shift

TAG_MESSAGE=""
DO_PUSH=false
AUTO_YES=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        -m)
            TAG_MESSAGE="$2"
            shift 2
            ;;
        --push)
            DO_PUSH=true
            shift
            ;;
        --yes|-y)
            AUTO_YES=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            usage
            ;;
    esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
CMAKE_FILE="$ROOT_DIR/CMakeLists.txt"

# Extract current version from CMakeLists.txt
CURRENT_VERSION=$(grep -oE 'project\(edinz VERSION [0-9]+\.[0-9]+\.[0-9]+' "$CMAKE_FILE" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
if [[ -z "$CURRENT_VERSION" ]]; then
    echo "Error: Could not find version in CMakeLists.txt"
    exit 1
fi

IFS='.' read -r MAJOR MINOR PATCH <<< "$CURRENT_VERSION"

case "$BUMP_TYPE" in
    major)
        MAJOR=$((MAJOR + 1))
        MINOR=0
        PATCH=0
        ;;
    minor)
        MINOR=$((MINOR + 1))
        PATCH=0
        ;;
    patch)
        PATCH=$((PATCH + 1))
        ;;
    *)
        echo "Error: bump type must be 'major', 'minor', or 'patch'"
        usage
        ;;
esac

NEW_VERSION="$MAJOR.$MINOR.$PATCH"
TAG_NAME="v$NEW_VERSION"

if [[ -z "$TAG_MESSAGE" ]]; then
    TAG_MESSAGE="Release $TAG_NAME"
fi

echo "Bumping version: $CURRENT_VERSION -> $NEW_VERSION"

# Check for uncommitted changes (beyond what we're about to do)
if ! git -C "$ROOT_DIR" diff --quiet HEAD 2>/dev/null; then
    echo "Warning: You have uncommitted changes. They will be included in the release commit."
    if ! $AUTO_YES; then
        read -rp "Continue? [y/N] " confirm
        if [[ "$confirm" != [yY] ]]; then
            echo "Aborted."
            exit 1
        fi
    fi
fi

# Check if tag already exists
if git -C "$ROOT_DIR" rev-parse "$TAG_NAME" >/dev/null 2>&1; then
    echo "Error: Tag $TAG_NAME already exists"
    exit 1
fi

# Update version in CMakeLists.txt
sed -i.bak "s/project(edinz VERSION $CURRENT_VERSION /project(edinz VERSION $NEW_VERSION /" "$CMAKE_FILE" && rm -f "$CMAKE_FILE.bak"

# Verify the replacement actually worked
if grep -q "project(edinz VERSION $NEW_VERSION " "$CMAKE_FILE"; then
    echo "Updated CMakeLists.txt"
else
    echo "Error: Failed to update version in CMakeLists.txt"
    exit 1
fi

# Regenerate version header via cmake
if [[ -d "$ROOT_DIR/build" ]]; then
    cmake -DSOURCE_DIR="$ROOT_DIR" -DBINARY_DIR="$ROOT_DIR/build" \
        -DPROJECT_VERSION="$NEW_VERSION" \
        -DPROJECT_VERSION_MAJOR="$MAJOR" \
        -DPROJECT_VERSION_MINOR="$MINOR" \
        -DPROJECT_VERSION_PATCH="$PATCH" \
        -P "$ROOT_DIR/cmake/GenerateVersion.cmake"
    echo "Regenerated version header"
fi

# Commit and tag
cd "$ROOT_DIR"
git add -A
git commit -m "chore: bump version to $NEW_VERSION"
git tag -a "$TAG_NAME" -m "$TAG_MESSAGE"

echo "Created commit and tag: $TAG_NAME"

if $DO_PUSH; then
    git push origin HEAD
    git push origin "$TAG_NAME"
    echo "Pushed commit and tag to origin"
else
    echo ""
    echo "To push: git push origin HEAD && git push origin $TAG_NAME"
fi
