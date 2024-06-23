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

  auto ModuleSource = Wasm->CreateSource(watap::source_info { WasmBin });

  auto Runtime = Wasm->CreateInstance(watap::instance_info {
    .ModuleSource = ModuleSource,
    .ImportTable = nullptr,
  });

  auto TestFn = [&]( const std::string_view Name, std::span<const watap::value> Arguments, watap::value ExpectedResult )
  {
    auto Result = Runtime->Call(Name, Arguments);

    std::cout << std::format(
      "{}:\n"
      "  Result   : {}\n"
      "  Expected : {}\n",
      Name,
      Result ? std::to_string(Result->F32x4[0]) : "Unknown",
      ExpectedResult.F32x4[0]
    );
  };

  TestFn("f_inv_sqrt", std::array<watap::value, 1> {
    watap::value {.F32x4 { 47.0F }},

  }, watap::value {.F32x4 { std::sqrt(1.0F / 47.0F) }});

  TestFn("vec3f_len", std::array<watap::value, 3> {
      watap::value { .F32x4 {3.0F} },
      watap::value { .F32x4 {4.0F} },
      watap::value { .F32x4 {5.0F} },

  }, watap::value {.F32x4 { std::sqrt(50.0F) }});

  TestFn("f_sign", std::array<watap::value, 1> {
    watap::value {.F32x4 { 47.0F }},
  }, watap::value {.F32x4 { 1.0F }});

  TestFn("f_sign", std::array<watap::value, 1> {
    watap::value {.F32x4 { -47.0F }},
  }, watap::value {.F32x4 { -1.0F }});

  Wasm->DestroyInstance(Runtime);
  Wasm->DestroySource(ModuleSource);

  watap::impl::standard::Destroy(Wasm);

  return 0;
}