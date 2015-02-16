//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include <vector>
#include <sstream>
#include <RoundTripDataPoint.h>


namespace TestHarness
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();

	private:
		void testButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		double InternetConnectSocketAsync();
	};
}

#pragma once
#include <functional>
#ifdef __cplusplus_winrt
#include <windows.h>
#endif

namespace pplpp {
	namespace details
	{
		class ContextCallback
		{
#ifdef __cplusplus_winrt

		public:
			~ContextCallback()
			{
				reset();
			}

			ContextCallback()
			{
				m_context = nullptr;
			}

			ContextCallback(const ContextCallback& src)
			{
				assign(src.m_context);
			}

			ContextCallback(ContextCallback&& src)
			{
				m_context = src.m_context;
				src.m_context = nullptr;
			}

			ContextCallback& operator=(const ContextCallback& src)
			{
				if (this != &src)
				{
					reset();
					assign(src.m_context);
				}
				return *this;
			}

			ContextCallback& operator=(ContextCallback&& src)
			{
				if (this != &src)
				{
					reset();
					m_context = src.m_context;
					src.m_context = nullptr;
				}
				return *this;
			}

			void capture()
			{
				reset();
				if (isCurrentSTA())
				{
					HRESULT hr = CoGetObjectContext(IID_IContextCallback, reinterpret_cast<void **>(&m_context));
					if (FAILED(hr))
						m_context = nullptr;
				}
			}

			void callInContext(std::function<void()> func) const
			{
				if (m_context == nullptr)
				{
					func();
				}
				else
				{
					ComCallData callData;
					ZeroMemory(&callData, sizeof(callData));
					callData.pUserDefined = reinterpret_cast<void *>(&func);

					HRESULT hr = m_context->ContextCallback(&_bridge, &callData, IID_ICallbackWithNoReentrancyToApplicationSTA, 5, nullptr);
					if (FAILED(hr))
					{
						throw Platform::Exception::CreateException(hr);
					}
				}
			}

			bool operator==(const ContextCallback& right) const
			{
				return (m_context == right.m_context);
			}

			bool operator!=(const ContextCallback& right) const
			{
				return !(operator==(right));
			}

		private:

			void reset()
			{
				if (m_context != nullptr)
				{
					m_context->Release();
					m_context = nullptr;
				}
			}

			void assign(IContextCallback *context)
			{
				m_context = context;
				if (m_context != nullptr)
				{
					m_context->AddRef();
				}
			}

			static HRESULT __stdcall _bridge(ComCallData *_PParam)
			{
				std::function<void()> *pFunc = reinterpret_cast<std::function<void()> *>(_PParam->pUserDefined);
				(*pFunc)();
				return S_OK;
			}

			// Returns the origin information for the caller (runtime / Windows Runtime apartment as far as task continuations need know)
			bool isCurrentSTA()
			{
				APTTYPE _AptType;
				APTTYPEQUALIFIER _AptTypeQualifier;

				HRESULT hr = CoGetApartmentType(&_AptType, &_AptTypeQualifier);
				if (SUCCEEDED(hr))
				{
					// We determine the origin of a task continuation by looking at where .then is called, so we can tell whether
					// to need to marshal the continuation back to the originating apartment. If an STA thread is in executing in
					// a neutral aparment when it schedules a continuation, we will not marshal continuations back to the STA,
					// since variables used within a neutral apartment are expected to be apartment neutral.
					switch (_AptType)
					{
					case APTTYPE_MAINSTA:
					case APTTYPE_STA:
						return true;
					default:
						break;
					}
				}
				return false;
			}

			IContextCallback *m_context;
#else
		public:
			void callInContext(std::function<void()> func) const
			{
				func();
			}
			void capture()	{}
			bool operator==(const ContextCallback&) const
			{
				return true;
			}

			bool operator!=(const ContextCallback&) const
			{
				return false;
			}
#endif
		};
	}
}

#pragma once
#include <agents.h>

#include <functional>
#include <chrono>
#include <memory>
#include <atomic>
#include <ppl.h>


namespace pplpp
{
	namespace details
	{
		class TimerPoolImpl
		{
			class TimerWithCallback
			{
				concurrency::timer<int> m_timer;
				concurrency::call<int> m_callback;
				std::function<void(bool)> m_userCallback;
				concurrency::cancellation_token m_token;
				concurrency::cancellation_token_registration m_reg;
				std::atomic<bool> m_fired, m_constructed;

			public:
				TimerWithCallback(std::chrono::milliseconds timeout, std::function<void(bool) > callback, concurrency::cancellation_token token) : m_timer(static_cast<int>(timeout.count()), 0, NULL, false),
					m_callback([this](int) mutable { this->timerCallback(true); }), m_userCallback(callback), m_token(token), m_fired(false), m_constructed(false)
				{
					m_timer.link_target(&m_callback);
					m_timer.start();
					if (token != concurrency::cancellation_token::none())
						m_reg = token.register_callback([this]()mutable {this->timerCallback(false); });
					m_constructed = true;
				}

				void timerCallback(bool firedbytimer)
				{
					if (m_fired.exchange(true))
						return;

					if (firedbytimer)
					{
						if (m_token != concurrency::cancellation_token::none())
							m_token.deregister_callback(m_reg);
					}
					else
						m_timer.stop();

					struct AsyncDeletor
					{
						TimerWithCallback *m_target;
						AsyncDeletor(TimerWithCallback *target) : m_target(target) {}
						~AsyncDeletor()
						{
							auto t = m_target;
							concurrency::create_task([=] {
								while (!t->m_constructed); // busy wait for constructor completion
								delete t;
							});
						}
					} deletor(this);

					m_userCallback(firedbytimer);
				}
			};
		public:
			void queue_timer_callback(std::chrono::milliseconds timeout, std::function<void(bool)> callback, concurrency::cancellation_token token = concurrency::cancellation_token::none())
			{
				// Memory will be cleand by itself
				new TimerWithCallback(timeout, callback, token);
			}
		};
	}
	typedef details::TimerPoolImpl timer_pool_t;
};

#pragma once

#include <memory>
#include <chrono>
#include <atomic>
#include <vector>
#include <mutex>
#include <functional>
#include <type_traits>

namespace pplpp
{
	namespace details
	{
		static void iterative_task_impl(concurrency::task_completion_event<void> finished, std::function<concurrency::task<bool>()> body,
			concurrency::cancellation_token ct, concurrency::cancellation_token_source cts, concurrency::task_continuation_context context = concurrency::task_continuation_context::use_default())
		{
			if (ct.is_canceled())
			{
				cts.cancel();
				return;
			}
			body().then([=](concurrency::task<bool> previous) {
				try {
					if (previous.get())
						iterative_task_impl(finished, body, ct, cts, context);
					else
						finished.set();
				}
				catch (concurrency::task_canceled) {
					cts.cancel();
				}
				catch (...) {
					finished.set_exception(std::current_exception());
				}
			}, ct, context);
		}
	} // namespace details


	/// <summary>
	///     Creates a timer task that will complete after <paramref name="time"/>.
	/// </summary>
	/// <param name="time">
	///     The timeout for the returning task.
	/// </param>
	/// <param name="ct">
	///     The cancellation token for cancelling timer.
	/// </param>
	/// <returns>
	///     It will return a task that completes after <paramref name="time"/>.
	/// </returns>
	inline concurrency::task<void> create_timer_task(std::chrono::milliseconds time, concurrency::cancellation_token ct = concurrency::cancellation_token::none())
	{
		concurrency::task_completion_event<void> tce;

		// Offload cancellation to a task 
		concurrency::cancellation_token_source asyncTokenSrc;
		if (ct != concurrency::cancellation_token::none())
		{
			ct.register_callback([=] {
				concurrency::create_task([=] {
					asyncTokenSrc.cancel();
				});
			});
			ct = asyncTokenSrc.get_token();
		}

		timer_pool_t().queue_timer_callback(time, [tce](bool isComplete) {
			if (isComplete)
				tce.set();
		}, ct);
		return concurrency::create_task(tce, ct);
	}

	/// <summary>
	///     <c>cancellation_token_source</c> with delayed cancelation feature. <c>cancel</c> method
	///     in this class has a overload that will cancel this <c>cancellation_token_source</c> after a delay.
	/// </summary>
	class timed_cancellation_token_source
	{
		concurrency::cancellation_token_source m_tokenSource;
	public:

		/// <summary>
		///      Cancel <c>cancellation_token_source</c> and all tokens associated with it
		///      after <paramref name="delay"/> time.
		/// </summary>
		void cancel()
		{
			m_tokenSource.cancel();
		}

		/// <summary>
		///      Cancel <c>cancellation_token_source</c> and all tokens associated with it
		///      after <paramref name="delay"/> time.
		/// </summary>
		/// <param name="delay">
		///     The delay time for the cancelation action.
		/// </param>
		void cancel(std::chrono::milliseconds delay)
		{
			auto tokenSource = m_tokenSource; // add a ref-count
			timer_pool_t().queue_timer_callback(delay, [tokenSource](bool) {
				tokenSource.cancel();
			});
		}

		/// <summary>
		///     Returns a <c>cancellation_token</c> associated with this source.
		/// </summary>
		/// <returns>
		///     A cancellation token associated with this source.
		/// </returns>
		/// <remarks>
		///     You can poll the returned token for cancellation or provide a callback to be invoked when cancellation occurs.</para>
		///     <para>For more information, see <see cref="cancellation_token Class"/>.</para>
		/// </remarks>
		concurrency::cancellation_token get_token() const
		{
			return m_tokenSource.get_token();
		}
	};

	/// <summary>
	///     Creates a task iteratively execute user Functor. During the process, each new iteration will be the continuation of the
	///     last iteration's returning task, and the process will keep going on until the Boolean value from returning task becomes False.
	/// </summary>
	/// <param name="body">
	///     The user Functor used as loop body. It is required to return a task with type bool, which used as predictor that decides
	///     whether the loop needs to be continued.
	/// </param>
	/// <param name="ct">
	///     The cancellation token linked to the iterative task.
	/// </param>
	/// <returns>
	///     The task that will perform the asynchronous iterative execution.
	/// </returns>
	/// <remarks>
	///     This function dynamically creates a long chain of continuations by iteratively concating tasks created by user Functor <paramref name="body"/>,
	///     The iteration will not stop until the result of the returning task from user Functor <paramref name="body"/> is <c> False </c>.
	/// </remarks>
	inline concurrency::task<void> create_iterative_task(std::function<concurrency::task<bool>()> body,
		concurrency::task_continuation_context context = concurrency::task_continuation_context::use_default(), concurrency::cancellation_token ct = concurrency::cancellation_token::none())
	{
		concurrency::task_completion_event<void> finished;
		concurrency::cancellation_token_source cts;
		auto runnable = [=] {
			try {
				details::iterative_task_impl(finished, body, ct, cts, context);
			}
			catch (...) {
				finished.set_exception(std::current_exception());
			}
		};
#ifdef __cplusplus_winrt
		if (context == concurrency::task_continuation_context::use_current())
			runnable();
		else
#endif
			concurrency::create_task(runnable);

		return concurrency::create_task(finished, cts.get_token());
	}


#ifdef __cplusplus_winrt
	/// <summary>
	///     a <c>concurrency::task<ResultType></c> class with progress reporter feature.
	/// </summary>
	/// <typeparam name="ResultType">
	///     The result type of this task.
	/// </typeparam>
	/// <typeparam name="ProgressType">
	///     The progress_reporter type.
	/// </typeparam>
	/// <remarks>
	///     Most of the features on this class is identical to <c>concurrency::task<ResultType></c>,
	///     except this class will be able to provide progress information about the underlying
	///     async object.
	/// </remarks>
	template <typename ResultType, typename ProgressType>
	class task_with_progress
	{
		template <typename T>
		struct VoidToInt { typedef T type; };
		template <>
		struct VoidToInt<void> { typedef int type; };

		class ProgressEvent
		{
			std::vector<std::function<void(ProgressType)>> m_callbacks;
			std::mutex m_lock;

		public:
			void _FireProgress(ProgressType progress)
			{
				std::unique_lock<std::mutex> scopedLock(m_lock);
				for (size_t i = 0; i < m_callbacks.size(); i++)
					m_callbacks[i](progress);
			}

			void registerCallback(std::function<void(ProgressType)> callback)
			{
				details::ContextCallback context;
				context.capture();
				std::unique_lock<std::mutex> scopedLock(m_lock);

				m_callbacks.push_back([context, callback](ProgressType progress) {
					context.callInContext([&] {
						callback(progress);
					});
				});
			}
		};

		std::shared_ptr<ProgressEvent> m_progressEvent;
		concurrency::progress_reporter<ProgressType> m_reporter;
		concurrency::task<ResultType> m_task;

	public:
		typedef ResultType result_type;
		typedef ProgressType progress_type;

		/// <summary>
		///     Subscribe progress report by registering callback functor.
		/// </summary>
		/// <param name="callback">
		///     The callback functor which will be invoked each time progress fired.
		///     The signature of this functor must be <c>void (ProgressType)</c>.
		/// </param>
		/// <remarks>
		///     The callback function will be called the same apartment as <c>on_progress</c> called.
		///     This function could be invoked multiple times to make multiple subscriptions.
		/// </remarks>
		void on_progress(std::function<void(ProgressType)> callback)
		{
			m_progressEvent->registerCallback(callback);
		}

		/// <summary>
		///     Construct a <c>task_with_progress</c> object from a functor taking progress_reporter.
		/// </summary>
		/// <param name="func">
		///     The user functor that will be scheduled on this task. This functor must provide progress information.
		///     The signature of this functor must be <c>ResultType(concurrency::progress_reporter<ProgressType>)</c>.
		/// </param>
		/// <param name="token">
		///     The cancellation token linked to the new created task.
		/// </param>
		task_with_progress(std::function<ResultType(concurrency::progress_reporter<ProgressType>)> func, concurrency::cancellation_token token = concurrency::cancellation_token::none()) :
			m_progressEvent(std::make_shared<ProgressEvent>()),
			m_reporter(concurrency::progress_reporter<ProgressType>::_CreateReporter(m_progressEvent))
		{
			auto reporter = m_reporter;
			m_task = create_task([=] { return func(reporter); }, token);
		}

		/// <summary>
		///     Construct a <c>task_with_progress</c> object from <c>IAsyncActionWithProgress</c>.
		/// </summary>
		/// <param name="arg">
		///     The <c>IAsyncActionWithProgress</c> async object, from which progress information is subscribled.
		/// </param>
		/// <param name="token">
		///     The cancellation token linked to the new created task.
		/// </param>
		task_with_progress(Windows::Foundation::IAsyncActionWithProgress<ProgressType>^ arg, concurrency::cancellation_token token = concurrency::cancellation_token::none()) :
			m_progressEvent(std::make_shared<ProgressEvent>()),
			m_reporter(concurrency::progress_reporter<ProgressType>::_CreateReporter(m_progressEvent)), m_task(arg, token)
		{
			auto reporter = m_reporter;
			arg->Progress = ref new Windows::Foundation::AsyncActionProgressHandler<ProgressType>([=](Windows::Foundation::IAsyncActionWithProgress<ProgressType>^, ProgressType p) {
				reporter.report(p);
			});
		}

		/// <summary>
		///     Construct a <c>task_with_progress</c> object from <c>IAsyncOperationWithProgress</c>.
		/// </summary>
		/// <param name="arg">
		///     The <c>IAsyncOperationWithProgress</c> async object, from which progress information is subscribled.
		/// </param>
		/// <param name="token">
		///     The cancellation token linked to the new created task.
		/// </param>
		task_with_progress(Windows::Foundation::IAsyncOperationWithProgress<typename VoidToInt<ResultType>::type, ProgressType>^ arg, concurrency::cancellation_token token = concurrency::cancellation_token::none()) :
			m_progressEvent(std::make_shared<ProgressEvent>()),
			m_reporter(concurrency::progress_reporter<ProgressType>::_CreateReporter(m_progressEvent)), m_task(arg, token)
		{
			auto reporter = m_reporter;
			arg->Progress = ref new Windows::Foundation::AsyncOperationProgressHandler<ResultType, ProgressType>([=](Windows::Foundation::IAsyncOperationWithProgress<ResultType, ProgressType>^, ProgressType p) {
				reporter.report(p);
			});
		}

		/// <summary>
		///     Default convert <c>task_with_progress<ResultType, ProgressType></c> to <c>task<ResultType></c>
		/// </summary>
		/// <remarks>
		///     After converting to task<ResultType>, the progress information will be lost and there is no way to convert back.
		/// </remarks>
		operator concurrency::task<ResultType>() const
		{
			return m_task;
		}

#pragma region forwarding
		template<typename Function>
		auto then(Function && func) const -> decltype(m_task.then(std::forward<Function>(func)))
		{
			return m_task.then(std::forward<Function>(func));
		}

		template<typename Function>
		auto then(Function && func, concurrency::cancellation_token token) const -> decltype(m_task.then(std::forward<Function>(func), token))
		{
			return m_task.then(std::forward<Function>(func), token);
		}

		template<typename Function>
		auto then(Function && func, concurrency::task_continuation_context context) const -> decltype(m_task.then(std::forward<Function>(func), context))
		{
			return m_task.then(std::forward<Function>(func), context);
		}
		concurrency::task_status wait() const
		{
			return m_task.wait();
		}
		ResultType get() const
		{
			return m_task.get();
		}
		bool is_apartment_aware() const
		{
			return m_task.is_apartment_aware();
		}
#pragma endregion forwarding
	};


	namespace details
	{
		template <typename ResultType, typename ProgressType>
		task_with_progress<ResultType, ProgressType> create_task_with_progress_impl(ResultType(*func) (concurrency::progress_reporter<ProgressType>), concurrency::cancellation_token token)
		{
			return task_with_progress<ResultType, ProgressType>(func, token);
		}

		template <typename ClassType, typename ResultType, typename ProgressType>
		auto functorTraits(ResultType(ClassType::*) (concurrency::progress_reporter<ProgressType>) const)->task_with_progress<ResultType, ProgressType>;

		template <typename ClassType, typename ResultType, typename ProgressType>
		auto functorTraits(ResultType(ClassType::*) (concurrency::progress_reporter<ProgressType>))->task_with_progress<ResultType, ProgressType>;

		template <typename Functor>
		auto create_task_with_progress_impl(Functor func, concurrency::cancellation_token token) -> decltype(functorTraits(&Functor::operator()))
		{
			return decltype(functorTraits(&Functor::operator()))(func, token);
		}

	}

	/// <summary>
	///     Creates a PPL <c>task_with_progress</c> object. 
	///     <c>create_task_with_progress</c> can be used anywhere you would have used a create_task_with_progress constructor.
	///     It is provided mainly for convenience, because it allows use of the <c>auto</c> keyword while creating tasks.
	/// </summary>
	/// <param name="action">
	///     The <c>IAsyncActionWithProgress</c> from which the task_with_progress is to be constructed.
	/// </param>
	/// <param name="token">
	///     The cancellation token to associate with the task. When the source for this token is canceled, cancellation will be requested on the task.
	/// </param>
	/// <returns>
	///     A new task_with_progress<void, ProgressType> instance, and its type is inferred from <paramref name="action"/>.
	/// </returns>
	template <typename ProgressType>
	task_with_progress<void, ProgressType> create_task_with_progress(Windows::Foundation::IAsyncActionWithProgress<ProgressType>^ action, concurrency::cancellation_token token = concurrency::cancellation_token::none())
	{
		return task_with_progress<void, ProgressType>(action, token);
	}

	/// <summary>
	///     Creates a PPL <c>task_with_progress</c> object. 
	///     <c>create_task_with_progress</c> can be used anywhere you would have used a create_task_with_progress constructor.
	///     It is provided mainly for convenience, because it allows use of the <c>auto</c> keyword while creating tasks.
	/// </summary>
	/// <param name="operation">
	///     The <c>IAsyncOperationWithProgress</c> from which the task_with_progress is to be constructed.
	/// </param>
	/// <param name="token">
	///     The cancellation token to associate with the task. When the source for this token is canceled, cancellation will be requested on the task.
	/// </param>
	/// <returns>
	///     A new task_with_progress<ResultType, ProgressType> instance, and its type is inferred from <paramref name="operation"/>.
	/// </returns>
	template <typename ResultType, typename ProgressType>
	task_with_progress<ResultType, ProgressType> create_task_with_progress(Windows::Foundation::IAsyncOperationWithProgress<ResultType, ProgressType>^ operation, concurrency::cancellation_token token = concurrency::cancellation_token::none())
	{
		return task_with_progress<ResultType, ProgressType>(operation, token);
	}


	/// <summary>
	///     Creates a PPL <c>task_with_progress</c> object. 
	///     <c>create_task_with_progress</c> can be used anywhere you would have used a create_task_with_progress constructor.
	///     It is provided mainly for convenience, because it allows use of the <c>auto</c> keyword while creating tasks.
	/// </summary>
	/// <param name="func">
	///     The functor from which the task_with_progress is to be constructed.
	///     The signature of this functor should be <c>ResultType(concurrency::progress_reporter<ProgressType>)</c>.
	/// </param>
	/// <param name="token">
	///     The cancellation token to associate with the task. When the source for this token is canceled, cancellation will be requested on the task.
	/// </param>
	/// <returns>
	///     A new task_with_progress<ResultType, ProgressType> instance, and its type is inferred from <paramref name="func"/>.
	/// </returns>
	template <typename Functor>
	auto create_task_with_progress(Functor func, concurrency::cancellation_token token = concurrency::cancellation_token::none()) -> decltype (details::create_task_with_progress_impl(func, token))
	{
		return details::create_task_with_progress_impl(func, token);
	}
#endif
}
