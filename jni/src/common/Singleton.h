#pragma once
#include <memory>

namespace nar
{
    template <class T> class Singleton {
      public:
        static void Create() {
            if (!instance) // Create new instance if none exists
                instance = std::make_shared<T>();
        }

        static std::shared_ptr<T> Get() {
            if (!instance) // Create new instance if none exists
                Create();
            return std::weak_ptr<T>(instance).lock(); // Return weak_ptr to not increase ref count
        }

        static void Dispose() {
            instance.reset(); // Delete instance if want to force it
        }

      private:
        inline static std::shared_ptr<T> instance;
    };
}