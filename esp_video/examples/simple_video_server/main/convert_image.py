from PIL import Image
import numpy as np
from PIL import Image, ImageOps
import os

# Use a clear, cropped picture of your face. 240x240 saves flash space.
target_folder = r"C:\Users\chuny\OneDrive - HKUST Connect\Desktop\fyp\face\main"
for image in os.listdir(target_folder):
    if image.endswith(".jpg") or image.endswith(".png"):
        img = Image.open(os.path.join(target_folder, image)
                         ).resize((240, 240)).convert("RGB")
        # img = Image.open(os.path.join(target_folder, image)
        #                  ).convert("RGB")
        with open(os.path.join(target_folder, image.split('.')[0] + ".rgb"), "wb") as f:
            f.write(img.tobytes())
        # print(f"Saved {image.split('.')[0]}.rgb")

        # 2. Force a perfect 240x240 square without squishing (crops edges if needed)
        # The LANCZOS filter ensures high-quality downsampling so details aren't lost
        # img_ready = ImageOps.fit(img, (240, 240), Image.Resampling.LANCZOS)

        # 3. Ensure it is strict 3-channel RGB
        # img_rgb = img_ready.convert("RGB")

        # # 4. Save to raw binary format
        # with open(os.path.join(target_folder, image.split('.')[0] + ".rgb"), "wb") as f:
        #     f.write(img_rgb.tobytes())

        # print(
        #     f"Success! Saved {image.split('.')[0]}.rgb. File size should be exactly 172,800 bytes.\n")


def verify_rgb_tensor(rgb_file_path):
    print(f"Verifying {rgb_file_path}...")

    with open(rgb_file_path, "rb") as f:
        raw_data = f.read()

    # 1. Verify exact byte count
    expected_bytes = 240 * 240 * 3
    if len(raw_data) != expected_bytes:
        print(
            f"ERROR: File is {len(raw_data)} bytes. Expected {expected_bytes} bytes.")
        return

    # 2. Reshape back to image array
    try:
        image_array = np.frombuffer(
            raw_data, dtype=np.uint8).reshape((240, 240, 3))
        img = Image.fromarray(image_array, 'RGB')

        # 3. Display the image
        img.show()
        print("Success: Image structure is intact. Check the popup window.")

    except Exception as e:
        print(f"ERROR reshaping tensor: {e}")


# Test the file you just generated
verify_rgb_tensor(
    r"C:\Users\chuny\OneDrive - HKUST Connect\Desktop\fyp\face\main\yiu.rgb")
