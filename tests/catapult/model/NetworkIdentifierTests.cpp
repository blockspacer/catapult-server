/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "catapult/model/NetworkIdentifier.h"
#include "catapult/utils/HexParser.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"
#include "tests/test/nodeps/Equality.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

#define TEST_CLASS NetworkIdentifierTests

	// region NetworkIdentifier - parsing

	TEST(TEST_CLASS, CanParseValidNetworkIdentifierValue) {
		// Arrange:
		auto assertSuccessfulParse = [](const auto& input, const auto& expectedParsedValue) {
			test::AssertParse(input, expectedParsedValue, [](const auto& str, auto& parsedValue) {
				return TryParseValue(str, parsedValue);
			});
		};

		// Assert:
		assertSuccessfulParse("mijin", NetworkIdentifier::Mijin);
		assertSuccessfulParse("mijin-test", NetworkIdentifier::Mijin_Test);
		assertSuccessfulParse("public", NetworkIdentifier::Public);
		assertSuccessfulParse("public-test", NetworkIdentifier::Public_Test);

		assertSuccessfulParse("0", static_cast<NetworkIdentifier>(0));
		assertSuccessfulParse("17", static_cast<NetworkIdentifier>(17));
		assertSuccessfulParse("255", static_cast<NetworkIdentifier>(255));
	}

	TEST(TEST_CLASS, CannotParseInvalidNetworkIdentifierValue) {
		test::AssertEnumParseFailure("mijin", NetworkIdentifier::Public, [](const auto& str, auto& parsedValue) {
			return TryParseValue(str, parsedValue);
		});
		test::AssertFailedParse("256", NetworkIdentifier::Public, [](const auto& str, auto& parsedValue) {
			return TryParseValue(str, parsedValue);
		});
	}

	// endregion

	// region UniqueNetworkFingerprint - constructor

	TEST(TEST_CLASS, UniqueNetworkFingerprint_CanCreateDefault) {
		// Act:
		auto fingerprint = UniqueNetworkFingerprint();

		// Assert:
		EXPECT_EQ(NetworkIdentifier::Zero, fingerprint.Identifier);
		EXPECT_EQ(GenerationHash(), fingerprint.GenerationHash);
	}

	TEST(TEST_CLASS, UniqueNetworkFingerprint_CanCreateAroundNetworkIdentifier) {
		// Act:
		auto fingerprint = UniqueNetworkFingerprint(static_cast<NetworkIdentifier>(17));

		// Assert:
		EXPECT_EQ(static_cast<NetworkIdentifier>(17), fingerprint.Identifier);
		EXPECT_EQ(GenerationHash(), fingerprint.GenerationHash);
	}

	TEST(TEST_CLASS, UniqueNetworkFingerprint_CanCreateAroundNetworkIdentifierAndNetworkGenerationHash) {
		// Act:
		auto fingerprint = UniqueNetworkFingerprint(static_cast<NetworkIdentifier>(17), GenerationHash{ { 1, 4, 9 } });

		// Assert:
		EXPECT_EQ(static_cast<NetworkIdentifier>(17), fingerprint.Identifier);
		EXPECT_EQ((GenerationHash{ { 1, 4, 9 } }), fingerprint.GenerationHash);
	}

	// endregion

	// region UniqueNetworkFingerprint - equality

	namespace {
		std::unordered_set<std::string> GetEqualTags() {
			return { "default", "copy" };
		}

		std::unordered_map<std::string, UniqueNetworkFingerprint> GenerateEqualityInstanceMap() {
			return {
				{ "default", { NetworkIdentifier::Mijin, GenerationHash{ { 1, 2, 3 } } } },
				{ "copy", { NetworkIdentifier::Mijin, GenerationHash{ { 1, 2, 3 } } } },

				{ "diff-identifier", { NetworkIdentifier::Mijin_Test, GenerationHash{ { 1, 2, 3 } } } },
				{ "diff-hash", { NetworkIdentifier::Mijin, GenerationHash{ { 1, 2, 4 } } } }
			};
		}
	}

	TEST(TEST_CLASS, UniqueNetworkFingerprint_OperatorEqualReturnsTrueOnlyForEqualValues) {
		test::AssertOperatorEqualReturnsTrueForEqualObjects("default", GenerateEqualityInstanceMap(), GetEqualTags());
	}

	TEST(TEST_CLASS, UniqueNetworkFingerprint_OperatorNotEqualReturnsTrueOnlyForUnequalValues) {
		test::AssertOperatorNotEqualReturnsTrueForUnequalObjects("default", GenerateEqualityInstanceMap(), GetEqualTags());
	}

	// endregion

	// region UniqueNetworkFingerprint - to string

	TEST(TEST_CLASS, UniqueNetworkFingerprint_CanOutput) {
		// Arrange:
		constexpr auto Generation_Hash_String = "272C4ECC55B7A42A07478A9550543C62673D1599A8362CC662E019049B76B7F2";
		auto fingerprint = UniqueNetworkFingerprint(
				NetworkIdentifier::Mijin_Test,
				utils::ParseByteArray<GenerationHash>(Generation_Hash_String));

		// Act:
		auto str = test::ToString(fingerprint);

		// Assert:
		EXPECT_EQ("Mijin_Test::272C4ECC55B7A42A07478A9550543C62673D1599A8362CC662E019049B76B7F2", str);
	}

	// endregion
}}
