class BlurModifier : public Modifier
{
public:
	float mV;
	int mArea;
	bool mR_en, mG_en, mB_en;
	bool mNegate;
	int mOnce;

	BlurModifier()
	{
		mNegate = false;
		mV = 1;
		mOnce = 0;
		mArea = 3;
		mR_en = true;
		mG_en = true;
		mB_en = true;
	}

	virtual int ui()
	{
		int ret = 0;
		ImGui::PushID(mUnique);

		if (!mOnce)
		{
			ImGui::OpenNextNode(1);
			mOnce = 1;
		}

		if (ImGui::CollapsingHeader("Blur Modifier"))
		{
			ret = common();
			if (ImGui::SliderFloat("##Strength", &mV, 0, 1.0f)) { gDirty = 1; }	ImGui::SameLine();
			if (ImGui::Button("Reset##strength   ")) { gDirty = 1; mV = 1; } ImGui::SameLine();
			ImGui::Text("Strength");

			if (ImGui::SliderInt("##kernel area", &mArea, 2, 32)) { gDirty = 1; }	ImGui::SameLine();
			if (ImGui::Button("Reset##kernel area   ")) { gDirty = 1; mArea = 2; } ImGui::SameLine();
			ImGui::Text("Kernel area");
			
			if (ImGui::Checkbox("Red enable", &mR_en)) { gDirty = 1; } ImGui::SameLine();
			if (ImGui::Checkbox("Green enable", &mG_en)) { gDirty = 1; } ImGui::SameLine();
			if (ImGui::Checkbox("Blue enable", &mB_en)) { gDirty = 1; } ImGui::SameLine();
			if (ImGui::Checkbox("Subtract blurred from source", &mNegate)) { gDirty = 1; }
		}
		ImGui::PopID();
		return ret;
	}

	virtual void process()
	{
		float ra = 0, ga = 0, ba = 0;
		int i, j, c;
		float * buf = new float[192 * 256 * 3 * 3];
		float * rbuf = buf + (256 * 192 * 0);
		float * gbuf = buf + (256 * 192 * 1);
		float * bbuf = buf + (256 * 192 * 2);

		for (j = 0, c = 0; j < 192; j++)
		{
			ra = ga = ba = 0;
			for (i = 0; i < 256; i++, c++)
			{
				ra += gBitmapProcFloat[c * 3 + 2];
				ga += gBitmapProcFloat[c * 3 + 1];
				ba += gBitmapProcFloat[c * 3 + 0];

				if (j == 0)
				{
					rbuf[c] = ra;
					gbuf[c] = ga;
					bbuf[c] = ba;
				}
				else
				{
					rbuf[c] = ra + rbuf[c - 256];
					gbuf[c] = ga + gbuf[c - 256];
					bbuf[c] = ba + bbuf[c - 256];
				}
			}
		}

		for (j = 0, c = 0; j < 192; j++)
		{
			for (i = 0; i < 256; i++, c++)
			{
				int x1 = i - mArea / 2;
				int y1 = j - mArea / 2;
				int x2 = x1 + mArea - 1;
				int y2 = y1 + mArea - 1;
				int div = (x2 - x1) * (y2 - y1);
				if (x1 < 0) x1 = 0;
				if (x1 > 255) x1 = 255;
				if (x2 < 0) x2 = 0;
				if (x2 > 255) x2 = 255;
				if (y1 < 0) y1 = 0;
				if (y1 > 191) y1 = 191;
				if (y2 < 0) y2 = 0;
				if (y2 > 191) y2 = 191;

				y1 *= 256;
				y2 *= 256;
				float r = rbuf[y2 + x2] - rbuf[y1 + x2] - rbuf[y2 + x1] + rbuf[y1 + x1];
				float g = gbuf[y2 + x2] - gbuf[y1 + x2] - gbuf[y2 + x1] + gbuf[y1 + x1];
				float b = bbuf[y2 + x2] - bbuf[y1 + x2] - bbuf[y2 + x1] + bbuf[y1 + x1];

				r /= div;
				g /= div;
				b /= div;

				if (mNegate)
				{
					if (mB_en) gBitmapProcFloat[c * 3 + 0] -= b * mV;
					if (mG_en) gBitmapProcFloat[c * 3 + 1] -= g * mV;
					if (mR_en) gBitmapProcFloat[c * 3 + 2] -= r * mV;
				}
				else
				{
					if (mB_en) gBitmapProcFloat[c * 3 + 0] += (b - gBitmapProcFloat[c * 3 + 0]) * mV;
					if (mG_en) gBitmapProcFloat[c * 3 + 1] += (g - gBitmapProcFloat[c * 3 + 1]) * mV;
					if (mR_en) gBitmapProcFloat[c * 3 + 2] += (r - gBitmapProcFloat[c * 3 + 2]) * mV;
				}
			}
		}

	}
};