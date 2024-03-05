#include "watap.h"

using namespace watap::common_types;

std::optional<std::vector<UINT8>> ReadBinary( std::string_view Path )
{
  std::fstream File {Path.data(), std::fstream::in | std::fstream::binary};

  if (!File)
  {
    std::cout << "Can't open file " << Path << std::endl;
    return std::nullopt;
  }

  std::vector<UINT8> Data;

  File.seekg(0, std::fstream::end);
  Data.resize(static_cast<SIZE_T>(File.tellg()));
  File.seekg(0, std::fstream::beg);

  File.read((CHAR *)Data.data(), Data.size());

  return std::move(Data);
}

INT main( INT Argc, const CHAR **Argv )
{
  std::vector<UINT8> WasmBin;
  if (auto Opt = ReadBinary(Argc > 1 ? Argv[1] : "example/math.wasm"))
    WasmBin = *Opt;
  else
    return 0;

  auto Wasm = watap::impl::standard::Create();

  auto ModuleSource = Wasm->CreateModuleSource(watap::module_source_info { WasmBin });


  auto Runtime = Wasm->CreateRuntime(watap::runtime_info {
    .ModuleSource = ModuleSource,
    .ImportTable = nullptr,
  });

  {
    auto Result = Runtime->Call("f_inv_sqrt", std::array<watap::value, 3> {
      watap::value { .F32x4 {47.0F} },
    });

    if (!Result)
      std::cout << "No result\n";
    else
      std::cout << std::format(
        "f_inv_sqrt:\n"
        "  Result   : {}\n"
        "  Expected : {}\n",
        Result->F32x4[0],
        1.0F / std::sqrt(47.0F)
      );
  }

  {
    auto Result = Runtime->Call("vec3f_len", std::array<watap::value, 3> {
      watap::value { .F32x4 {3.0F} },
      watap::value { .F32x4 {4.0F} },
      watap::value { .F32x4 {5.0F} },
    });

    if (!Result)
      std::cout << "No result\n";
    else
      std::cout << std::format(
        "vec3f_len:\n"
        "  Result   : {}\n"
        "  Expected : {}\n",
        Result->F32x4[0],
        std::sqrt(50.0F)
      );
  }

  Wasm->DestroyRuntime(Runtime);
  Wasm->DestroyModuleSource(ModuleSource);

  watap::impl::standard::Destroy(Wasm);

  return 0;
}