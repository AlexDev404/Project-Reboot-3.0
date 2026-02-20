{
  "variables": {
    "project_reboot_build%": "<!(node -p \"process.env.PROJECT_REBOOT_BUILD || '0'\")"
  },
  "targets": [
    {
      "target_name": "icarus",
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "sources": [
        "src/addon.cpp",
        "src/FWorldBinding.cpp",
        "src/FPawnBinding.cpp",
        "src/FGameBinding.cpp",
        "src/FStormBinding.cpp",
        "src/FInventoryBinding.cpp",
        "src/FAdminBinding.cpp",
        "src/FBotsBinding.cpp",
        "src/FlareBinding.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include_dir\")",
        "<!(node -p \"require('node-addon-api').include_dir\")",
        "../..",
        "src"
      ],
      "defines": [ 
        "NAPI_DISABLE_CPP_EXCEPTIONS",
        "NODE_ADDON_API_ENABLE_MAYBE"
      ],
      "conditions": [
        ["project_reboot_build=='1'", {
          "defines": [
            "PROJECT_REBOOT_BUILD"
          ],
          "include_dirs": [
            "../../..",
            "../../../vendor"
          ]
        }],
        ["OS=='win'", {
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 1,
              "AdditionalOptions": [ "/EHsc" ],
              "RuntimeLibrary": 2
            }
          },
          "defines": [
            "_HAS_EXCEPTIONS=1",
            "WIN32",
            "_WINDOWS"
          ]
        }],
        ["OS=='linux'", {
          "cflags_cc": [
            "-std=c++17",
            "-fexceptions"
          ]
        }],
        ["OS=='mac'", {
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LIBRARY": "libc++",
            "MACOSX_DEPLOYMENT_TARGET": "10.15",
            "CLANG_CXX_LANGUAGE_STANDARD": "c++17"
          }
        }]
      ]
    }
  ]
}
