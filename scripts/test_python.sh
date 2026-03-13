#!/bin/bash
# Test script for Python bindings
# Requires Python 3.9+ with pip

set -e

echo "=== SOMTParser Python Bindings Test ==="
echo ""

# Check Python version
PYTHON_VERSION=$(python3 --version 2>&1 | cut -d' ' -f2 | cut -d'.' -f1,2)
REQUIRED_VERSION="3.9"

if [[ "$(printf '%s\n' "$REQUIRED_VERSION" "$PYTHON_VERSION" | sort -V | head -n1)" != "$REQUIRED_VERSION" ]]; then
    echo "Error: Python $REQUIRED_VERSION or higher is required (found $PYTHON_VERSION)"
    echo "Please install Python 3.9+ and try again."
    exit 1
fi

echo "Python version: $PYTHON_VERSION"
echo ""

# Navigate to project root
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_DIR"

echo "Project directory: $PROJECT_DIR"
echo ""

# Install the package
echo "Installing somtparser..."
pip install -v ".[test]"

echo ""
echo "Running tests..."
pytest -v test/python/

echo ""
echo "=== All tests passed! ==="
