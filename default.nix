# { lib
# , stdenv
# , fetchFromGitHub
# , alacritty
# , cage
# , cairo
# , libxkbcommon
# , makeWrapper
# , mesa
# , meson
# , ninja
# , pkg-config
# , udev
# , wayland
# , wayland-protocols
# , wlroots
# , xwayland
# }:
{ ... }:
with import <nixpkgs> {};

stdenv.mkDerivation rec {
  pname = "wio-dev";
  version = "0.pre+unstable=2021-06-27";

  src = ./src.tar;
  nativeBuildInputs = [
    makeWrapper
    meson
    ninja
    pkg-config
  ];

  buildInputs = [
    cairo
    libxkbcommon
    mesa # for libEGL
    udev
    wayland
    wayland-protocols
    wlroots
    xwayland
  ];

  postInstall = ''
    mv wio wio-dev
    wrapProgram $out/bin/wio-dev \
      --prefix PATH ":" "${lib.makeBinPath [ alacritty cage ]}"
  '';

  meta = with lib; {
    homepage = "https://wio-project.org/";
    description = "That Plan 9 feel, for Wayland";
    longDescription = ''
      Wio is a Wayland compositor for Linux and FreeBSD which has a similar look
      and feel to plan9's rio.
    '';
    license = licenses.mit;
    # maintainers = with maintainers; [ AndersonTorres ];
    inherit (wayland.meta) platforms;
  };

  passthru.providedSessions = [ "wio-dev" ];
}
# TODO: factor Linux-specific options
