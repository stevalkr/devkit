rec {
  description = "Cpp Environment With ROS";

  inputs = {
    nixgl.url   = github:guibou/nixGL;
    rospkgs.url = github:lopsided98/nix-ros-overlay;
  };

  outputs = { self, nixgl, rospkgs }:
  let
    distro = "noetic";
    system = "x86_64-linux";
    ros  = rospkgs.legacyPackages.${system}.${distro};
    pkgs = rospkgs.inputs.nixpkgs.legacyPackages.${system}.extend nixgl.overlays.default;
  in
  {
    devShells.${system}.default =
      let

        mkShell = pkgs.mkShell.override
          {
            stdenv = pkgs.stdenvAdapters.useMoldLinker pkgs.stdenv;
            # stdenv = pkgs.stdenvAdapters.useMoldLinker pkgs.llvmPackages_16.stdenv;
          };

        rosPkgs = if distro == "noetic" then with ros; [
                      catkin
                      rosbag
                      rosbash
                      rosnode
                      roslaunch
                      rospy
                      rviz
                    ]
                  else with ros; [
                      ros-core
                      rviz2
                    ];

      in
      mkShell
        {
          nativeBuildInputs = [
            pkgs.mold
            pkgs.ninja
            pkgs.meson
            pkgs.ccache
            pkgs.clang-tools_16
            pkgs.llvmPackages_16.openmp

            pkgs.pyright
            pkgs.python311

            pkgs.fmt
            pkgs.spdlog
            pkgs.yaml-cpp

            pkgs.glibcLocales
            (ros.buildEnv {
              paths = rosPkgs; })
          ];

          BOOST_LIBRARYDIR = "${pkgs.lib.getLib pkgs.boost}/lib";
          BOOST_INCLUDEDIR = "${pkgs.lib.getDev pkgs.boost}/include";

          ROS_HOSTNAME = "localhost";
          ROS_MASTER_URI = "http://localhost:11311";

          shellHook = ''
            echo ${description}
          '';
        };
  };

  nixConfig = {
    extra-substituters = [ "https://ros.cachix.org" ];
    extra-trusted-public-keys = [ "ros.cachix.org-1:dSyZxI8geDCJrwgvCOHDoAfOm5sV1wCPjBkKL+38Rvo=" ];
  };
}
