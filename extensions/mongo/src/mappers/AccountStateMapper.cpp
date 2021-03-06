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

#include "AccountStateMapper.h"
#include "MapperUtils.h"
#include "catapult/state/AccountState.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace mongo { namespace mappers {

	// region ToDbModel

	namespace {
		auto& StreamAccountImportanceSnapshots(bson_stream::document& builder, const state::AccountImportanceSnapshots& snapshots) {
			auto importancesArray = builder << "importances" << bson_stream::open_array;
			for (const auto& snapshot : snapshots) {
				if (model::ImportanceHeight(0) == snapshot.Height)
					break;

				importancesArray
						<< bson_stream::open_document
							<< "value" << ToInt64(snapshot.Importance)
							<< "height" << ToInt64(snapshot.Height)
						<< bson_stream::close_document;
			}

			importancesArray << bson_stream::close_array;
			return builder;
		}

		auto& StreamAccountActivityBuckets(bson_stream::document& builder, const state::AccountActivityBuckets& buckets) {
			auto activityBucketsArray = builder << "activityBuckets" << bson_stream::open_array;
			for (const auto& bucket : buckets) {
				if (model::ImportanceHeight(0) == bucket.StartHeight)
					break;

				activityBucketsArray
						<< bson_stream::open_document
							<< "startHeight" << ToInt64(bucket.StartHeight)
							<< "totalFeesPaid" << ToInt64(bucket.TotalFeesPaid)
							<< "beneficiaryCount" << static_cast<int32_t>(bucket.BeneficiaryCount)
							<< "rawScore" << static_cast<int64_t>(bucket.RawScore)
						<< bson_stream::close_document;
			}

			activityBucketsArray << bson_stream::close_array;
			return builder;
		}

		auto& StreamAccountBalances(bson_stream::document& builder, const state::AccountBalances& balances) {
			auto mosaicsArray = builder << "mosaics" << bson_stream::open_array;
			for (const auto& entry : balances)
				StreamMosaic(mosaicsArray, entry.first, entry.second);

			mosaicsArray << bson_stream::close_array;
			return builder;
		}
	}

	bsoncxx::document::value ToDbModel(const state::AccountState& accountState) {
		bson_stream::document builder;
		builder
				<< "account" << bson_stream::open_document
					<< "address" << ToBinary(accountState.Address)
					<< "addressHeight" << ToInt64(accountState.AddressHeight)
					<< "publicKey" << ToBinary(accountState.PublicKey)
					<< "publicKeyHeight" << ToInt64(accountState.PublicKeyHeight)
					<< "accountType" << utils::to_underlying_type(accountState.AccountType)
					<< "linkedAccountKey" << ToBinary(accountState.LinkedAccountKey);
		StreamAccountImportanceSnapshots(builder, accountState.ImportanceSnapshots);
		StreamAccountActivityBuckets(builder, accountState.ActivityBuckets);
		StreamAccountBalances(builder, accountState.Balances);
		builder << bson_stream::close_document;
		return builder << bson_stream::finalize;
	}

	// endregion
}}}
