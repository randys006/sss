#include <memory>

#include "../../pax/pax.h"

#include "CppUnitTest.h"

using namespace std;
using namespace sss::pax;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace paxmstest
{		
	TEST_CLASS(PaxTests)
	{
	public:
		
		TEST_METHOD(basicWriteReadModify)
		{
            Logger::WriteMessage("Starting basicWriteReadModify");

            vector<float> floatData { 158.98166f, 171.61903f, 160.06989f, 148.83504f };
            floatRasterFile floatFile{ 2, 2, static_cast<void*>(floatData.data()) };
            float piVal = 3.1416f;
            floatFile.addMetaVal("pi", piVal);
            Assert::AreEqual(piVal,         floatFile.getMetaFloat("pi"));

            string floatFileName{ "floatFile.pax" };
            paxBufPtr floatBuf;
            floatFile.writeToBuffer(floatBuf);
            auto floatFileSize = floatBuf->size();
            rasterFileBase::writeToFile(floatBuf, floatFileName);

            paxBufPtr floatInBuf = rasterFileBase::readFile(floatFileName);
            auto floatInFileSize = floatInBuf->size();

            Assert::AreEqual(floatFileSize, floatInFileSize);

            floatRasterFile floatInFile;
            auto importRet = floatInFile.import(floatInBuf);
            Assert::AreEqual(importRet, static_cast<int>(PAX_OK));

            // verify data was imported correctly
            Assert::AreEqual(158.98166f,    floatInFile.floatValXY(0, 0));
            Assert::AreEqual(171.61903f,    floatInFile.floatValXY(1, 0));
            Assert::AreEqual(160.06989f,    floatInFile.floatValRC(1, 0));
            Assert::AreEqual(148.83504f,    floatInFile.floatValRC(1, 1));
            Assert::AreEqual(piVal,         floatInFile.getMetaFloat("pi"));

            // tweak values and verify again
            double piPrecise = 3.1415926535897932;
            floatInFile.addMetaVal("pi", piPrecise);    // overwrites the old pi with double type
            Assert::AreNotEqual(piVal,  floatInFile.getMetaFloat("pi")); // getMetaFloat() returns garbage
            Assert::AreEqual(piPrecise,     floatInFile.getMetaDouble("pi"));
        }

	};
}
