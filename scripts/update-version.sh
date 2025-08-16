#!/bin/bash

# Update version numbers in project files
# Usage: ./scripts/update-version.sh [version]
# If no version is provided, it will auto-determine based on conventional commits

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Get the script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"

cd "$PROJECT_ROOT"

# Get current version from git tags
get_current_version() {
    local latest_tag=$(git tag -l "v*.*.*" | sort -V | tail -n1)
    if [[ -z "$latest_tag" ]]; then
        echo "0.0.0"
    else
        echo "${latest_tag#v}"
    fi
}

# Determine next version based on conventional commits
determine_next_version() {
    local current_version=$1
    IFS='.' read -r major minor patch <<< "$current_version"
    
    # Get the latest tag or first commit
    local latest_tag=$(git tag -l "v*.*.*" | sort -V | tail -n1)
    if [[ -z "$latest_tag" ]]; then
        compare_from=$(git rev-list --max-parents=0 HEAD)
    else
        compare_from="$latest_tag"
    fi
    
    # Check for breaking changes
    if git log ${compare_from}..HEAD --pretty=format:"%B" | grep -q "BREAKING CHANGE:\|^[a-z]*!:"; then
        print_info "Found breaking changes - major version bump"
        major=$((major + 1))
        minor=0
        patch=0
    # Check for new features
    elif git log ${compare_from}..HEAD --pretty=format:"%s" | grep -qE "^feat(\(.*\))?:"; then
        print_info "Found new features - minor version bump"
        minor=$((minor + 1))
        patch=0
    # Check for fixes
    elif git log ${compare_from}..HEAD --pretty=format:"%s" | grep -qE "^fix(\(.*\))?:"; then
        print_info "Found bug fixes - patch version bump"
        patch=$((patch + 1))
    else
        print_info "Other changes - patch version bump"
        patch=$((patch + 1))
    fi
    
    echo "${major}.${minor}.${patch}"
}

# Update version in CMakeLists.txt
update_cmake_version() {
    local version=$1
    local file="$PROJECT_ROOT/CMakeLists.txt"
    
    if [[ -f "$file" ]]; then
        # Extract major.minor from version
        IFS='.' read -r major minor patch <<< "$version"
        cmake_version="${major}.${minor}"
        
        # Update the VERSION line in project()
        sed -i.bak "s/VERSION [0-9]\+\.[0-9]\+/VERSION ${cmake_version}/" "$file"
        rm -f "${file}.bak"
        print_info "Updated CMakeLists.txt to version ${cmake_version}"
    else
        print_warn "CMakeLists.txt not found"
    fi
}

# Update version in source code
update_source_version() {
    local version=$1
    local file="$PROJECT_ROOT/source/app/application.cpp"
    
    if [[ -f "$file" ]]; then
        # Update version string in application
        sed -i.bak "s/Liar's Dice v[0-9]\+\.[0-9]\+\.[0-9]\+/Liar's Dice v${version}/" "$file"
        # Also check for shortened version format
        sed -i.bak "s/Liar's Dice v[0-9]\+\.[0-9]\+/Liar's Dice v${version}/" "$file"
        rm -f "${file}.bak"
        print_info "Updated application.cpp to version ${version}"
    else
        print_warn "application.cpp not found"
    fi
}

# Update version in README
update_readme_version() {
    local version=$1
    local file="$PROJECT_ROOT/README.md"
    
    if [[ -f "$file" ]]; then
        # Update version badges and references
        sed -i.bak "s/version-[0-9]\+\.[0-9]\+\.[0-9]\+/version-${version}/g" "$file"
        sed -i.bak "s/v[0-9]\+\.[0-9]\+\.[0-9]\+/v${version}/g" "$file"
        rm -f "${file}.bak"
        print_info "Updated README.md to version ${version}"
    else
        print_warn "README.md not found"
    fi
}

# Update CHANGELOG
update_changelog() {
    local version=$1
    local file="$PROJECT_ROOT/CHANGELOG.md"
    
    if [[ -f "$file" ]]; then
        local date=$(date +%Y-%m-%d)
        
        # Check if Unreleased section exists
        if grep -q "## \[Unreleased\]" "$file"; then
            # Add new version section after Unreleased
            sed -i.bak "/## \[Unreleased\]/a\\
\\
## [${version}] - ${date}" "$file"
            rm -f "${file}.bak"
            print_info "Added version ${version} to CHANGELOG.md"
        else
            print_warn "No [Unreleased] section found in CHANGELOG.md"
        fi
    else
        print_warn "CHANGELOG.md not found"
    fi
}

# Main script
main() {
    local version=$1
    
    if [[ -z "$version" ]]; then
        # Auto-determine version
        current_version=$(get_current_version)
        print_info "Current version: ${current_version}"
        
        version=$(determine_next_version "$current_version")
        print_info "Next version will be: ${version}"
    else
        # Remove 'v' prefix if present
        version="${version#v}"
        print_info "Using provided version: ${version}"
    fi
    
    # Validate version format
    if ! [[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        print_error "Invalid version format. Expected: X.Y.Z"
        exit 1
    fi
    
    # Update files
    print_info "Updating project files..."
    update_cmake_version "$version"
    update_source_version "$version"
    update_readme_version "$version"
    update_changelog "$version"
    
    print_info "Version update complete!"
    print_info "Next steps:"
    echo "  1. Review the changes: git diff"
    echo "  2. Commit the changes: git add -A && git commit -m \"chore: bump version to ${version}\""
    echo "  3. Tag the release: git tag v${version}"
    echo "  4. Push to remote: git push origin main --tags"
}

# Run main function
main "$@"