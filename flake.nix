{
  description = "flutter dev env";
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  outputs = { self, nixpkgs }: {
    devShell.x86_64-linux = 
      let 
        pkgs = import nixpkgs { 
          system = "x86_64-linux";
          config = {
            android_sdk.accept_license = true;
            allowUnfree = true;
          };
        };
        
        androidComposition = pkgs.androidenv.composeAndroidPackages {
          cmdLineToolsVersion = "11.0";
          platformToolsVersion = "36.0.2";
          buildToolsVersions = [ "35.0.0" "34.0.0" "33.0.2" "30.0.3" "28.0.3" ];
          includeEmulator = false;
          platformVersions = [ "36" "35" "34" "33" "28" ];
          includeSources = false;
          includeSystemImages = false;
          includeNDK = true;
          ndkVersions = ["26.1.10909125"];
          extraLicenses = [
            "android-sdk-license"
            "android-sdk-preview-license"
            "android-googletv-license"
            "android-sdk-arm-dbt-license"
            "google-gdk-license"
            "intel-android-extra-license"
            "mips-android-sysimage-license"
          ];
        };
        
        androidSdk = androidComposition.androidsdk;
        nixAndroidSdk = "${androidSdk}/libexec/android-sdk";   # read-only Nix path
        writableSdk  = "$HOME/.local/share/android-sdk-nix";   # writable mirror
        
      in pkgs.mkShell {
        buildInputs = [
          pkgs.flutter
          pkgs.jdk17
          androidSdk
        ];
        
        # Point Flutter to the WRITABLE mirror, not the Nix store
        JAVA_HOME = "${pkgs.jdk17}";
        
        shellHook = ''
          export ANDROID_HOME="${writableSdk}"
          export ANDROID_SDK_ROOT="${writableSdk}"

          # --- Build the writable SDK mirror ---
          mkdir -p "$ANDROID_HOME/licenses"

          # Symlink every SDK component except licenses (we write those ourselves)
          for item in ${nixAndroidSdk}/*; do
            name=$(basename "$item")
            if [[ "$name" != "licenses" && ! -e "$ANDROID_HOME/$name" ]]; then
              ln -s "$item" "$ANDROID_HOME/$name"
            fi
          done

          # Mirror ALL licenses from the read-only Nix SDK first (covers google-gdk-license, etc.)
          if [[ -d "${nixAndroidSdk}/licenses" ]]; then
            cp -f ${nixAndroidSdk}/licenses/* "$ANDROID_HOME/licenses/"
            chmod u+w "$ANDROID_HOME/licenses/"*
          fi

          # Overwrite the two main hashes with the known-good values Flutter expects
          printf "\n8933bad161af4178b1185d1a37fbf41ea5269c55\nd56f5187479451eabf01fb78af6dfcb131a6481e\n24333f8a63b6825ea9c5514f83c2829b004d1fee" \
            > "$ANDROID_HOME/licenses/android-sdk-license"

          printf "\n84831b9409646a918e30573bab4c9c91346d8abd" \
            > "$ANDROID_HOME/licenses/android-sdk-preview-license"

          # --- PATH and Flutter config ---
          export PATH="$ANDROID_HOME/tools:$ANDROID_HOME/tools/bin:$ANDROID_HOME/platform-tools:$PATH"

          flutter config --android-sdk "$ANDROID_SDK_ROOT" --no-analytics

          echo "────────────────────────────────────"
          echo "Android SDK : $ANDROID_SDK_ROOT"
          echo "Java        : $JAVA_HOME"
          echo "────────────────────────────────────"
        '';
      };
  };
}
