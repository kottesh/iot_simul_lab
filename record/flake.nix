{
  description = "A Nix-flake-based environment for Pandoc to PDF generation";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      supportedSystems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];

      forEachSystem = nixpkgs.lib.genAttrs supportedSystems;
    in
    {
      packages = forEachSystem (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};

          tex = pkgs.texlive.combine {
            inherit (pkgs.texlive)
              scheme-medium
              needspace
              multirow
              framed
              fvextra
              upquote
              ;
          };
        in
        {
          default = pkgs.stdenvNoCC.mkDerivation {
            name = "record-pdf";
            src = ./.;

            nativeBuildInputs = [
              pkgs.pandoc
              tex
              pkgs.poppler-utils
            ];

            buildPhase = ''
              export HOME=$(mktemp -d)
              bash ./build.sh
            '';

            installPhase = ''
              mkdir -p $out
              cp output/record.pdf $out/
            '';
          };
        }
      );

      devShells = forEachSystem (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};

          tex = pkgs.texlive.combine {
            inherit (pkgs.texlive)
              scheme-medium
              needspace
              multirow
              framed
              fvextra
              upquote
              ;
          };
        in
        {
          default = pkgs.mkShell {
            packages = [
              pkgs.pandoc
              tex
              pkgs.poppler-utils
            ];
          };
        }
      );
    };
}
