#!/bin/bash

# Gas Detector App - Quick Start Script

echo "=== Gas Detector Flutter App ==="
echo ""

# Check if we're in the right directory
if [ ! -f "pubspec.yaml" ]; then
    echo "Error: Please run this script from the gas_detector_app directory"
    exit 1
fi

echo "1. Checking dependencies..."
if ! command -v flutter &> /dev/null; then
    echo "Flutter not found. Trying with nix develop..."
    USE_NIX=true
else
    USE_NIX=false
fi

echo ""
echo "2. Getting dependencies..."
if [ "$USE_NIX" = true ]; then
    nix develop -c flutter pub get
else
    flutter pub get
fi

echo ""
echo "3. Choose an option:"
echo "   1) Run app in debug mode"
echo "   2) Build APK (release)"
echo "   3) Run tests"
echo "   4) Clean build"
echo ""
read -p "Enter option (1-4): " option

case $option in
    1)
        echo ""
        echo "Starting app in debug mode..."
        if [ "$USE_NIX" = true ]; then
            nix develop -c flutter run
        else
            flutter run
        fi
        ;;
    2)
        echo ""
        echo "Building release APK..."
        if [ "$USE_NIX" = true ]; then
            nix develop -c flutter build apk --release
        else
            flutter build apk --release
        fi
        echo ""
        echo "APK built at: build/app/outputs/flutter-apk/app-release.apk"
        ;;
    3)
        echo ""
        echo "Running tests..."
        if [ "$USE_NIX" = true ]; then
            nix develop -c flutter test
        else
            flutter test
        fi
        ;;
    4)
        echo ""
        echo "Cleaning build..."
        if [ "$USE_NIX" = true ]; then
            nix develop -c flutter clean
        else
            flutter clean
        fi
        echo "Clean complete. Run option 1 or 2 to rebuild."
        ;;
    *)
        echo "Invalid option"
        exit 1
        ;;
esac
