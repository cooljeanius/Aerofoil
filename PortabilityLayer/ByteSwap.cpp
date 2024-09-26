#include "ByteSwap.h"
#include "CoreDefs.h"

namespace PortabilityLayer
{
	namespace ByteSwap
	{
		template<class TNumberType, class TUnsignedType>
		void SwapArbitraryBig(TNumberType &v)
		{
			GP_STATIC_ASSERT(sizeof(TNumberType) == sizeof(TUnsignedType));

			uint8_t bytes[sizeof(TNumberType)];
			for (size_t i = 0; i < sizeof(TNumberType); i++)
				bytes[i] = reinterpret_cast<const uint8_t*>(&v)[i];

			TUnsignedType result = 0;
			for (size_t i = 0; i < sizeof(TNumberType); i++)
				result |= static_cast<TUnsignedType>(bytes[i]) << (sizeof(TUnsignedType) * 8 - 8 - (i * 8));

			v = static_cast<TNumberType>(result);
		}

		template<class TNumberType, class TUnsignedType>
		void SwapArbitraryLittle(TNumberType &v)
		{
			GP_STATIC_ASSERT(sizeof(TNumberType) == sizeof(TUnsignedType));

			uint8_t bytes[sizeof(TNumberType)];
			for (size_t i = 0; i < sizeof(TNumberType); i++)
				bytes[i] = reinterpret_cast<const uint8_t*>(&v)[i];

			TUnsignedType result = 0;
			for (size_t i = 0; i < sizeof(TNumberType); i++)
				result |= static_cast<TUnsignedType>(bytes[i]) << (i * 8);

			v = static_cast<TNumberType>(result);
		}

		void BigInt16(int16_t &v)
		{
			SwapArbitraryBig<int16_t, uint16_t>(v);
		}

		void BigInt32(int32_t &v)
		{
			SwapArbitraryBig<int32_t, uint32_t>(v);
		}

		void BigInt64(int64_t &v)
		{
			SwapArbitraryBig<int64_t, uint64_t>(v);
		}

		void BigUInt16(uint16_t &v)
		{
			SwapArbitraryBig<uint16_t, uint16_t>(v);
		}

		void BigUInt32(uint32_t &v)
		{
			SwapArbitraryBig<uint32_t, uint32_t>(v);
		}

		void BigUInt64(uint64_t &v)
		{
			SwapArbitraryBig<uint64_t, uint64_t>(v);
		}

		void LittleInt16(int16_t &v)
		{
			SwapArbitraryLittle<int16_t, uint16_t>(v);
		}

		void LittleInt32(int32_t &v)
		{
			SwapArbitraryLittle<int32_t, uint32_t>(v);
		}

		void LittleInt64(int64_t &v)
		{
			SwapArbitraryLittle<int64_t, uint64_t>(v);
		}

		void LittleUInt16(uint16_t &v)
		{
			SwapArbitraryLittle<uint16_t, uint16_t>(v);
		}

		void LittleUInt32(uint32_t &v)
		{
			SwapArbitraryLittle<uint32_t, uint32_t>(v);
		}

		void LittleUInt64(uint64_t &v)
		{
			SwapArbitraryLittle<uint64_t, uint64_t>(v);
		}
	}
}
