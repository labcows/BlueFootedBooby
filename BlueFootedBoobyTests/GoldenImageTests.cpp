#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <vector>

#include "Raytracer.h"

namespace
{
	constexpr int kWidth  = 96;
	constexpr int kHeight = 96;
	constexpr int kSpp    = 16;
	constexpr int kDepth  = 8;
	constexpr int kChannels = 3;

	// Relative to the test project directory, which is the working directory
	// both in Visual Studio and in CI.
	const std::filesystem::path kGoldenDir = "golden";

	std::vector<unsigned char> renderToBytes()
	{
		Raytracer rt(kWidth, kHeight);
		std::vector<math::vec4> pixels(size_t(kWidth) * kHeight);
		rt.renderPathTracedTiled(pixels, kSpp, kDepth);

		std::vector<unsigned char> rgb(size_t(kWidth) * kHeight * kChannels);
		for (size_t i = 0; i < pixels.size(); i++)
		{
			// The render path already tone-maps and gamma-corrects into [0, 1].
			for (int c = 0; c < 3; c++)
			{
				const float v = math::clamp(pixels[i][c], 0.0f, 1.0f);
				rgb[i * 3 + c] = static_cast<unsigned char>(v * 255.0f + 0.5f);
			}
		}
		return rgb;
	}

	int countDifferingBytes(const unsigned char* a, const unsigned char* b, size_t count)
	{
		int diff = 0;
		for (size_t i = 0; i < count; i++)
			if (a[i] != b[i]) diff++;
		return diff;
	}

	void RenderAndSave()
	{
		std::filesystem::create_directories(kGoldenDir);

		const std::vector<unsigned char> rgb = renderToBytes();
		const std::filesystem::path out = kGoldenDir / "cornell_actual.png";

		const int ok = stbi_write_png(out.string().c_str(), kWidth, kHeight, kChannels,
			rgb.data(), kWidth * kChannels);

		EXPECT_NE(ok, 0) << "failed to write " << out.string();
		std::cout << "wrote " << std::filesystem::absolute(out).string() << std::endl;
	}
}

TEST(GoldenImage, CompareImageWithGolden)
{
	
	int w, h, channels;
	const char* path = "./golden/cornell_golden.png";

	std::unique_ptr<unsigned char[], decltype(&stbi_image_free)> data(stbi_load(path, &w, &h, &channels, kChannels), stbi_image_free);

	ASSERT_NE(data, nullptr)
		<< "failed to load " << std::filesystem::absolute(path).string()
		<< " (" << stbi_failure_reason() << ")";


	const std::vector<unsigned char> rgb = renderToBytes();

	ASSERT_EQ(w, kWidth);
	ASSERT_EQ(h, kHeight);
	ASSERT_EQ(w * h * 3, static_cast<int>(rgb.size()));

	int diff = 0;

	for (int i = 0; i < h; i++)
		for (int j = 0; j < w; j++)
		{
			for (int k = 0; k < 3; k++)
			{
				int idx = (i * w + j) * kChannels + k;
				if (data[idx] != rgb[idx])
					diff++;
			}
		}

	EXPECT_EQ(diff, 0);
}

TEST(GoldenImage, DetectsCorruptedPixels)
{
	const std::vector<unsigned char> rendered = renderToBytes();
	std::vector<unsigned char> failedBuffer = rendered;

	constexpr int kCorruptedBytes = 200;
	const size_t stride = failedBuffer.size() / kCorruptedBytes;   
	ASSERT_GT(stride, 0u);

	for (int i = 0; i < kCorruptedBytes; i++)
	{
		unsigned char& byte = failedBuffer[stride * i];
		byte = static_cast<unsigned char>(byte ^ 0xFF);
	}

	EXPECT_EQ(countDifferingBytes(rendered.data(), failedBuffer.data(), rendered.size()),
	          kCorruptedBytes);
}