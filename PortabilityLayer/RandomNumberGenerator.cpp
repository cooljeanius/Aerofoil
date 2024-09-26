#include "RandomNumberGenerator.h"

namespace PortabilityLayer
{
	class RandomNumberGeneratorMT19937 final : public RandomNumberGenerator
	{
	public:
		RandomNumberGeneratorMT19937();

		void Seed(uint32_t seed) override;
		uint32_t GetNextAndAdvance() override;

		static RandomNumberGeneratorMT19937 *GetInstance();

	private:
		static const uint32_t kA = 0x9908b0df;
		static const int kW = 32;
		static const int kN = 624;
		static const int kM = 397;
		static const int kR = 31;
		static const uint32_t kLowMask = (static_cast<uint32_t>(1) << kR) - 1;
		static const uint32_t kHighMask = ~kLowMask;

		void Twist();

		uint32_t m_state[kN];
		int m_index;

		static RandomNumberGeneratorMT19937 ms_instance;
	};

	RandomNumberGeneratorMT19937::RandomNumberGeneratorMT19937()
	{
		Seed(0x243F6A88);	// First 8 hex digits of pi
	}

	void RandomNumberGeneratorMT19937::Seed(uint32_t seed)
	{
		m_index = kN;

		m_state[0] = seed;
		for (unsigned int i = 1; i < kN; i++)
		{
			const uint32_t prev = m_state[i - 1];
			m_state[i] = static_cast<uint32_t>((static_cast<uint64_t>(1812433253) * static_cast<uint64_t>(prev ^ (prev >> 30)) + i) & 0xffffffff);
		}
	}

	uint32_t RandomNumberGeneratorMT19937::GetNextAndAdvance()
	{
		if (m_index == kN)
			Twist();

		uint32_t x = m_state[m_index++];
		x ^= (x >> 11);
		x ^= (x >> 7) & 0x9d2c5680;
		x ^= (x << 15) & 0xefc60000;
		return x ^ (x >> 18);
	}

	void RandomNumberGeneratorMT19937::Twist()
	{
		for (unsigned int i = 0; i < kN; i++)
		{
			uint32_t x = (m_state[i] & kHighMask) + (m_state[(i + 1) % kN] & kLowMask);
			uint32_t xA = x >> 1;
			if ((x & 1) == 1)
				xA ^= kA;
			m_state[i] = m_state[(i + kM) % kN] ^ kA;
		}
		m_index = 0;
	}

	RandomNumberGeneratorMT19937 *RandomNumberGeneratorMT19937::GetInstance()
	{
		return &ms_instance;
	}

	RandomNumberGeneratorMT19937 RandomNumberGeneratorMT19937::ms_instance;

	RandomNumberGenerator* RandomNumberGenerator::GetInstance()
	{
		return RandomNumberGeneratorMT19937::GetInstance();
	}
}
