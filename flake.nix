{
  inputs.nixpkgs.url = "github:nixos/nixpkgs";

  outputs = {
    self,
    nixpkgs,
  }: let
    pkgs = nixpkgs.legacyPackages.x86_64-linux;
  in {
    devShells.x86_64-linux.default = pkgs.mkShell {
      nativeBuildInputs = [
        pkgs.pkg-config
        pkgs.cmake
        pkgs.clang-tools
      ];
      buildInputs = with pkgs; [
        qt6.qtbase
        libcxx
        # qtcreator

        # this is for the shellhook portion
        qt6.wrapQtAppsHook
        makeWrapper
        bashInteractive

        # llvmPackages.clang-tools
        # clang
        # clang-tools_19
        gcc
        bear
      ];

      # set the environment variables that Qt apps expect
      shellHook = ''
        bashdir=$(mktemp -d)
        makeWrapper "$(type -p bash)" "$bashdir/bash" "''${qtWrapperArgs[@]}"
        exec "$bashdir/bash"
      '';
    };
  };
}
