rec {
  description = "Devkit Environment";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-24.05-darwin";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };
      in
      {
        apps.sk = {
          type = "app";
          program = "${self.packages.${system}.default}/bin/sk";
        };

        packages.default = pkgs.callPackage
          ({ lib, stdenv, lua, fmt, doctest, yaml-cpp, ninja, cmake, meson, pkg-config, installShellFiles }:
            stdenv.mkDerivation rec{
              pname = "devkit";
              version = "develop";

              src = ../.;
              dontUseCmakeConfigure = true;

              buildInputs = [ lua fmt doctest yaml-cpp ];
              nativeBuildInputs = [ ninja cmake meson pkg-config installShellFiles ];

              postInstall = ''
                installShellCompletion --fish ${src}/completions/sk.fish
              '';

              meta = with lib; {
                description = "devkit";
                license = licenses.mit;
              };
            })
          { };

        devShell = pkgs.mkShell {
          buildInputs = [
            pkgs.lua
            pkgs.fmt
            pkgs.doctest
            pkgs.yaml-cpp
          ];

          nativeBuildInputs = [
            pkgs.ninja
            pkgs.cmake
            pkgs.meson
            pkgs.pkg-config
            pkgs.ccache
            pkgs.nixd
            pkgs.nixpkgs-fmt
            pkgs.clang-tools
            pkgs.lua-language-server
          ];

          shellHook = ''
            echo ${description}
          '';
        };
      }
    );
}
