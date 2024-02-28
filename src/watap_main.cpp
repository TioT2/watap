#include "watap.h"

using namespace watap;

INT main( INT Argc, const CHAR **Argv )
{
  std::string_view WasmPath = "example/cgsg_forever.wasm";
  if (Argc > 1)
    WasmPath = Argv[1];

  std::fstream File {WasmPath.data(), std::fstream::in | std::fstream::binary};

  if (!File)
  {
    std::cout << "Can't open file " << WasmPath << std::endl;
    return 0;
  }

  std::vector<BYTE> WasmData;

  File.seekg(0, std::fstream::end);
  WasmData.resize(File.tellg());
  File.seekg(0, std::fstream::beg);

  File.read((CHAR *)WasmData.data(), WasmData.size());

  ParseSections(WasmData);

  return 0;
}