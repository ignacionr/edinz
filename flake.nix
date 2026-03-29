{
  description = "edinz - terminal UI application";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        # Override the default pkgs to allow unfree software
        pkgs = import nixpkgs {
          inherit system;
          config.allowUnfree = true;
        };
      in
      {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "edinz";
          version = "0.1.0";
          src = ./.;

          nativeBuildInputs = [ pkgs.cmake ];

          meta = {
            description = "Terminal UI application";
            mainProgram = "edinz";
          };
        };

        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            clang-tools
            cmake
            gdb
            gh
            git
            github-copilot-cli
            lldb
            ninja
            tmux
          ];

          # Use clang with C++23 support
          stdenv = pkgs.clangStdenv;
        };
      }
    );
}

