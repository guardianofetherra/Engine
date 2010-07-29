#pragma once

#include "VaultGenerated.h"

namespace Asset
{
  class AssetFile;
}

namespace Luna
{
  class DetailsFrame : public DetailsFrameGenerated
  {
  public:
    DetailsFrame( wxWindow* parent );

    void Populate( const Nocturnal::Path& path );
  };
}