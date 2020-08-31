#pragma once

#include "RE/Bethesda/BSStringPool.h"
#include "RE/Bethesda/CRC.h"

namespace RE
{
	namespace detail
	{
		template <class CharT, bool CS>
		class BSFixedString
		{
		public:
			using size_type = std::uint32_t;
			using value_type = CharT;
			using pointer = value_type*;
			using const_pointer = const value_type*;
			using reference = value_type&;
			using const_reference = const value_type&;

			constexpr BSFixedString() noexcept = default;

			inline BSFixedString(const BSFixedString& a_rhs) :
				_data(a_rhs._data)
			{
				try_acquire();
			}

			inline BSFixedString(BSFixedString&& a_rhs) :
				_data(a_rhs._data)
			{
				a_rhs._data = nullptr;
			}

			inline BSFixedString(const_pointer a_string)
			{
				if (a_string) {
					GetEntry<value_type>(_data, a_string, CS);
				}
			}

			template <
				class T,
				std::enable_if_t<
					std::conjunction_v<
						std::is_convertible<const T&, std::basic_string_view<value_type>>,
						std::negation<
							std::is_convertible<const T&, const_pointer>>>,
					int> = 0>
			inline BSFixedString(const T& a_string)
			{
				const auto view = static_cast<std::basic_string_view<value_type>>(a_string);
				if (!view.empty()) {
					GetEntry<value_type>(_data, view.data(), CS);
				}
			}

			inline ~BSFixedString() { try_release(); }

			inline BSFixedString& operator=(const BSFixedString& a_rhs)
			{
				if (this != std::addressof(a_rhs)) {
					try_release();
					_data = a_rhs._data;
					try_acquire();
				}
				return *this;
			}

			inline BSFixedString& operator=(BSFixedString&& a_rhs)
			{
				if (this != std::addressof(a_rhs)) {
					_data = a_rhs._data;
					a_rhs._data = nullptr;
				}
				return *this;
			}

			inline BSFixedString& operator=(const_pointer a_string)
			{
				try_release();
				if (a_string) {
					GetEntry<value_type>(_data, a_string, CS);
				}
				return *this;
			}

			template <
				class T,
				std::enable_if_t<
					std::conjunction_v<
						std::is_convertible<const T&, std::basic_string_view<value_type>>,
						std::negation<
							std::is_convertible<const T&, const_pointer>>>,
					int> = 0>
			inline BSFixedString& operator=(const T& a_string)
			{
				const auto view = static_cast<std::basic_string_view<value_type>>(a_string);
				try_release();
				if (!view.empty()) {
					GetEntry<value_type>(_data, view.data(), CS);
				}
				return *this;
			}

			[[nodiscard]] inline const_reference front() const noexcept { return data()[0]; }
			[[nodiscard]] inline const_reference back() const noexcept { return data()[size() - 1]; }

			[[nodiscard]] inline const_pointer data() const noexcept
			{
				const auto cstr = _data ? _data->data<value_type>() : nullptr;
				return cstr ? cstr : EMPTY;
			}

			[[nodiscard]] inline const_pointer c_str() const noexcept { return data(); }

			[[nodiscard]] constexpr operator std::basic_string_view<value_type>() const { return { c_str(), length() }; }

			[[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

			[[nodiscard]] constexpr size_type size() const noexcept { return _data ? _data->size() : 0; }
			[[nodiscard]] constexpr size_type length() const noexcept { return _data ? _data->length() : 0; }

			[[nodiscard]] inline friend bool operator==(const BSFixedString& a_lhs, const BSFixedString& a_rhs) noexcept
			{
				const auto leaf = [](const BSFixedString& a_elem) { return a_elem._data ? a_elem._data->leaf() : nullptr; };
				const auto lLeaf = leaf(a_lhs);
				const auto rLeaf = leaf(a_rhs);
				if (lLeaf == rLeaf) {
					return true;
				} else if (a_lhs.empty() && a_rhs.empty()) {
					return true;
				} else {
					return false;
				}
			}

			[[nodiscard]] inline friend bool operator!=(const BSFixedString& a_lhs, const BSFixedString& a_rhs) noexcept { return !(a_lhs == a_rhs); }

			[[nodiscard]] inline friend bool operator==(const BSFixedString& a_lhs, std::basic_string_view<value_type> a_rhs)
			{
				if (a_lhs.empty() && a_rhs.empty()) {
					return true;
				} else if (const auto length = a_lhs.length(); length != a_rhs.length()) {
					return false;
				} else {
					return strncmp(a_lhs.c_str(), a_rhs.data(), length) == 0;
				}
			}

			[[nodiscard]] inline friend bool operator!=(const BSFixedString& a_lhs, std::basic_string_view<value_type> a_rhs) { return !(a_lhs == a_rhs); }
			[[nodiscard]] inline friend bool operator==(std::basic_string_view<value_type> a_lhs, const BSFixedString& a_rhs) { return a_rhs == a_lhs; }
			[[nodiscard]] inline friend bool operator!=(std::basic_string_view<value_type> a_lhs, const BSFixedString& a_rhs) { return !(a_lhs == a_rhs); }

			[[nodiscard]] inline friend bool operator==(const BSFixedString& a_lhs, const_pointer a_rhs) { return a_lhs == std::basic_string_view<value_type>(a_rhs ? a_rhs : EMPTY); }
			[[nodiscard]] inline friend bool operator!=(const BSFixedString& a_lhs, const_pointer a_rhs) { return !(a_lhs == a_rhs); }
			[[nodiscard]] inline friend bool operator==(const_pointer a_lhs, const BSFixedString& a_rhs) { return a_rhs == a_lhs; }
			[[nodiscard]] inline friend bool operator!=(const_pointer a_lhs, const BSFixedString& a_rhs) { return !(a_lhs == a_rhs); }

		private:
			[[nodiscard]] static int strncmp(const char* a_lhs, const char* a_rhs, std::size_t a_length)
			{
				if constexpr (CS) {
					return std::strncmp(a_lhs, a_rhs, a_length);
				} else {
					return _strnicmp(a_lhs, a_rhs, a_length);
				}
			}

			[[nodiscard]] static int strncmp(const wchar_t* a_lhs, const wchar_t* a_rhs, std::size_t a_length)
			{
				if constexpr (CS) {
					return std::wcsncmp(a_lhs, a_rhs, a_length);
				} else {
					return _wcsnicmp(a_lhs, a_rhs, a_length);
				}
			}

			inline void try_acquire()
			{
				if (_data) {
					_data->acquire();
				}
			}

			inline void try_release() { BSStringPool::Entry::release(_data); }

			static constexpr const value_type EMPTY[]{ 0 };

			// members
			BSStringPool::Entry* _data{ nullptr };	// 0
		};

		extern template class BSFixedString<char, false>;
		extern template class BSFixedString<char, true>;
		extern template class BSFixedString<wchar_t, false>;
		extern template class BSFixedString<wchar_t, true>;
	}

	using BSFixedString = detail::BSFixedString<char, false>;
	using BSFixedStringCS = detail::BSFixedString<char, true>;
	using BSFixedStringW = detail::BSFixedString<wchar_t, false>;
	using BSFixedStringWCS = detail::BSFixedString<wchar_t, true>;

	template <class CharT, bool CS>
	struct BSCRC32<detail::BSFixedString<CharT, CS>>
	{
	public:
		[[nodiscard]] inline std::uint32_t operator()(const detail::BSFixedString<CharT, CS>& a_key) const noexcept
		{
			return BSCRC32<typename detail::BSFixedString<CharT, CS>::const_pointer>()(a_key.data());
		}
	};

	extern template struct BSCRC32<BSFixedString>;
	extern template struct BSCRC32<BSFixedStringCS>;
	extern template struct BSCRC32<BSFixedStringW>;
	extern template struct BSCRC32<BSFixedStringWCS>;

	class BGSLocalizedString
	{
	public:
		using size_type = typename BSFixedStringCS::size_type;
		using value_type = typename BSFixedStringCS::value_type;
		using pointer = typename BSFixedStringCS::pointer;
		using const_pointer = typename BSFixedStringCS::const_pointer;
		using reference = typename BSFixedStringCS::reference;
		using const_reference = typename BSFixedStringCS::const_reference;

		[[nodiscard]] inline const_pointer data() const noexcept { return _data.data(); }
		[[nodiscard]] inline const_pointer c_str() const noexcept { return _data.c_str(); }

		[[nodiscard]] inline bool empty() const noexcept { return _data.empty(); }

		[[nodiscard]] inline size_type size() const noexcept { return _data.size(); }
		[[nodiscard]] inline size_type length() const noexcept { return _data.length(); }

	private:
		// members
		BSFixedStringCS _data;	// 0
	};
	static_assert(sizeof(BGSLocalizedString) == 0x8);

	template <>
	struct BSCRC32<BGSLocalizedString>
	{
	public:
		[[nodiscard]] inline std::uint32_t operator()(const BGSLocalizedString& a_key) const noexcept
		{
			return BSCRC32<typename BGSLocalizedString::const_pointer>()(a_key.data());
		}
	};
}
