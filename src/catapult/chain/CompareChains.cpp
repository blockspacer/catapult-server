#include "CompareChains.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/thread/FutureUtils.h"
#include "catapult/utils/Casting.h"
#include <iostream>

namespace catapult { namespace chain {

	namespace {
		constexpr auto Num_Comparison_Functions = 2;
		constexpr auto Incomplete_Chain_Comparison_Code = static_cast<ChainComparisonCode>(-1);

		class CompareChainsContext : public std::enable_shared_from_this<CompareChainsContext> {
		public:
			CompareChainsContext(const api::ChainApi& local, const api::ChainApi& remote, const CompareChainsOptions& options)
					: m_local(local)
					, m_remote(remote)
					, m_options(options)
					, m_nextFunctionId(0) {
				m_comparisonFunctions[0] = [](auto& context) { return context.compareChainInfos(); };
				m_comparisonFunctions[1] = [](auto& context) { return context.compareHashes(); };
				// note that the last comparison function is guaranteed to complete
			}

		public:
			thread::future<CompareChainsResult> compare() {
				startNextCompare();
				return m_promise.get_future();
			}

		private:
			void startNextCompare() {
				m_comparisonFunctions[m_nextFunctionId++](*this).then([pThis = shared_from_this()](auto&& future) {
					if (pThis->isFutureChainComplete(future))
						return;

					pThis->startNextCompare();
				});
			}

			bool isFutureChainComplete(thread::future<ChainComparisonCode>& future) {
				try {
					auto code = future.get();
					if (Incomplete_Chain_Comparison_Code == code)
						return false;

					auto forkDepth = (m_localHeight - m_commonBlockHeight).unwrap();
					auto result = ChainComparisonCode::Remote_Is_Not_Synced == code
							? CompareChainsResult{ code, m_commonBlockHeight, forkDepth }
							: CompareChainsResult{ code, Height(static_cast<Height::ValueType>(-1)), 0 };
					m_promise.set_value(std::move(result));
					return true;
				} catch (...) {
					m_promise.set_exception(std::current_exception());
					return true;
				}
			}

			thread::future<ChainComparisonCode> compareChainInfos() {
				return thread::when_all(m_local.chainInfo(), m_remote.chainInfo()).then([pThis = shared_from_this()](
						auto&& aggregateFuture) {
					auto infoFutures = aggregateFuture.get();
					auto localInfo = infoFutures[0].get();
					auto remoteInfo = infoFutures[1].get();
					return pThis->compareChainInfos(localInfo, remoteInfo);
				});
			}

			ChainComparisonCode compareChainInfos(const api::ChainInfo& localInfo, const api::ChainInfo& remoteInfo) {
				if (isRemoteTooFarBehind(localInfo.Height, remoteInfo.Height))
					return ChainComparisonCode::Remote_Is_Too_Far_Behind;

				const auto& localScore = localInfo.Score;
				const auto& remoteScore = remoteInfo.Score;
				CATAPULT_LOG_LEVEL(localScore == remoteScore ? utils::LogLevel::Trace : utils::LogLevel::Debug)
						<< "comparing chain scores: " << localScore << " (local) vs " << remoteScore << " (remote)";

				if (remoteScore > localScore) {
					m_localHeight = localInfo.Height;
					return Incomplete_Chain_Comparison_Code;
				}

				return localScore == remoteScore
						? ChainComparisonCode::Remote_Reported_Equal_Chain_Score
						: ChainComparisonCode::Remote_Reported_Lower_Chain_Score;
			}

			bool isRemoteTooFarBehind(Height localHeight, Height remoteHeight) const {
				auto heightDifference = static_cast<int64_t>((localHeight - remoteHeight).unwrap());
				return heightDifference > m_options.MaxBlocksToRewrite;
			}

			thread::future<ChainComparisonCode> compareHashes() {
				auto localHeight = m_localHeight.unwrap();
				auto startingHeight = Height(localHeight > m_options.MaxBlocksToRewrite
						? localHeight - m_options.MaxBlocksToRewrite
						: 1);
				return thread::when_all(m_local.hashesFrom(startingHeight), m_remote.hashesFrom(startingHeight))
					.then([pThis = shared_from_this(), startingHeight](auto&& aggregateFuture) {
						auto hashesFuture = aggregateFuture.get();
						const auto& localHashes = hashesFuture[0].get();
						const auto& remoteHashes = hashesFuture[1].get();
						return pThis->compareHashes(startingHeight, localHashes, remoteHashes);
					});
			}

			ChainComparisonCode compareHashes(
					Height startingHeight,
					const model::HashRange& localHashes,
					const model::HashRange& remoteHashes) {
				// in order for this node to sync properly, the remote must return at least
				// rewrite-limit + 1 <= MaxBlocksToAnalyze hashes
				// larger MaxBlocksToAnalyze values can allow faster syncing by allowing the remote note to
				// return more hashes in one request
				if (remoteHashes.size() > m_options.MaxBlocksToAnalyze)
					return ChainComparisonCode::Remote_Returned_Too_Many_Hashes;

				// at least the first compared block should be the same; if not, the remote is a liar or on a fork
				auto firstDifferenceIndex = FindFirstDifferenceIndex(localHashes, remoteHashes);
				if (0 == firstDifferenceIndex)
					return ChainComparisonCode::Remote_Is_Forked;

				// if all the hashes match, the remote node lied because it can't have a higher score
				if (remoteHashes.size() == firstDifferenceIndex)
					return ChainComparisonCode::Remote_Lied_About_Chain_Score;

				m_commonBlockHeight = Height(startingHeight.unwrap() + firstDifferenceIndex - 1);
				m_localHeight = startingHeight + Height(localHashes.size() - 1);
				return ChainComparisonCode::Remote_Is_Not_Synced;
			}

		private:
			const api::ChainApi& m_local;
			const api::ChainApi& m_remote;
			CompareChainsOptions m_options;
			thread::promise<CompareChainsResult> m_promise;

			using ComparisonFunction = thread::future<ChainComparisonCode> (*)(CompareChainsContext& context);
			ComparisonFunction m_comparisonFunctions[Num_Comparison_Functions];
			size_t m_nextFunctionId;

			Height m_localHeight;
			Height m_commonBlockHeight;
		};
	}

	thread::future<CompareChainsResult> CompareChains(
			const api::ChainApi& local,
			const api::ChainApi& remote,
			const CompareChainsOptions& options) {
		auto pContext = std::make_shared<CompareChainsContext>(local, remote, options);
		return pContext->compare();
	}
}}