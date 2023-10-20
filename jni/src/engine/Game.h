#pragma once
#include "common/Singleton.h"

struct android_app;

namespace nar
{
    class Game : public Singleton<Game> {
      public:
        void Run(android_app *app);

      private:
        void Cleanup();
    };
}