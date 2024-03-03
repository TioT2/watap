#include "watap.h"

using namespace watap::common_types;

std::optional<std::vector<BYTE>> ReadBinary( std::string_view Path )
{
  std::fstream File {Path.data(), std::fstream::in | std::fstream::binary};

  if (!File)
  {
    std::cout << "Can't open file " << Path << std::endl;
    return std::nullopt;
  }

  std::vector<BYTE> Data;

  File.seekg(0, std::fstream::end);
  Data.resize(File.tellg());
  File.seekg(0, std::fstream::beg);

  File.read((CHAR *)Data.data(), Data.size());

  return std::move(Data);
}

INT main( INT Argc, const CHAR **Argv )
{
  std::vector<BYTE> WasmBin;
  if (auto Opt = ReadBinary(Argc > 1 ? Argv[1] : "example/math.wasm"))
    WasmBin = *Opt;
  else
    return 0;

  auto Wasm = watap::impl::standard::Create();

  auto ModuleSource = Wasm->CreateModuleSource(watap::module_source_info { WasmBin });

  Wasm->DestroyModuleSource(ModuleSource);

  watap::impl::standard::Destroy(Wasm);

  return 0;
}