#include <memory>

#include "../../pax/pax.h"

#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace sss::pax;
using namespace std;

namespace paxmstest
{		
	TEST_CLASS(PaxTests)
	{
	public:
		
		TEST_METHOD(basicWriteModifyRead)
		{
            vector<float> floatData { 1.0f, 2.0f, 3.0f, 4.0f };
            floatRasterFile floatFile{ 2, 2, static_cast<void*>(floatData.data()) };
            //floatFile.meta( "blah")
		}

	};
}
