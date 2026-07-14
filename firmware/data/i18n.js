(function () {
  "use strict";

  const en = {
    "Đang kết nối…":"Connecting…","Tổng quan":"Overview","Màn hình & bố cục":"Display & layout","Kiểu chữ":"Typography","Đèn RGB":"RGB lighting","Kết nối & thời gian":"Connectivity & time","Cập nhật trực tuyến":"Online updates","Ảnh khởi động":"Startup image","Hệ thống":"System",
    "KẾT NỐI MÁY TÍNH":"COMPUTER CONNECTION","Đang chờ Windows Agent":"Waiting for Windows Agent","Cắm cáp USB dữ liệu, mở PCScreen Windows Agent và chọn đúng cổng COM. Thẻ này tự ẩn khi nhận được dữ liệu.":"Connect the USB data cable, open PCScreen Windows Agent, and select the correct COM port. This card hides automatically when data arrives.","Sao chép chẩn đoán":"Copy diagnostics",
    "--% tải":"--% load","QUẠT CPU":"CPU FAN","QUẠT GPU":"GPU FAN","vòng/phút":"RPM","đang sử dụng":"in use","khung hình/giây":"frames/second","Ổ ĐĨA":"DISK","đã sử dụng":"in use","TẢI XUỐNG":"DOWNLOAD","TẢI LÊN":"UPLOAD","THỜI GIAN HOẠT ĐỘNG":"UPTIME","máy tính":"computer",
    "Tần suất cập nhật dữ liệu":"Telemetry update rate","Chọn tốc độ phù hợp giữa độ mượt và mức sử dụng tài nguyên của máy tính.":"Choose a rate that balances smooth updates and computer resource usage.","Chế độ":"Mode","Thời gian thực • 250 ms":"Real-time • 250 ms","Nhanh • 500 ms":"Fast • 500 ms","Cân bằng • 1 giây":"Balanced • 1 second","Tiết kiệm • 2 giây":"Power saving • 2 seconds","Tùy chỉnh":"Custom","Khoảng thời gian":"Interval","Tạm dừng cập nhật dữ liệu từ máy tính":"Pause telemetry updates from the computer","Đọc dữ liệu hiện tại":"Read current data",
    "Trạng thái thiết bị":"Device status","Kết nối trực tiếp":"Direct connection","Đang kết nối":"Connecting","Bộ nhớ khả dụng":"Available memory","Dung lượng lưu trữ":"Storage","Màn hình":"Display","Chu kỳ dữ liệu":"Data interval","1 giây":"1 second",
    "Giao diện tối ưu cho vỏ card đồ họa":"Interface optimized for a graphics card enclosure","Áp dụng nền đen, điểm nhấn đỏ, chữ tương phản cao và bố cục gọn cho màn hình 240×240.":"Apply a black background, red accents, high-contrast text, and a compact 240×240 layout.","Áp dụng giao diện MI50":"Apply MI50 interface","NHIỆT ĐỘ":"TEMPERATURE","HIỆU NĂNG":"PERFORMANCE",
    "Đèn nền màn hình":"Display backlight","Bật, tắt hoặc điều chỉnh độ sáng của màn hình.":"Turn the display on or off and adjust its brightness.","Độ sáng":"Brightness","Bật màn hình":"Turn display on","Tắt màn hình":"Turn display off",
    "Giao diện và chuyển trang":"Interface and page transitions","Ngôn ngữ":"Language","Tiếng Việt":"Vietnamese","Giao diện màu":"Color theme","Tối":"Dark","Sáng":"Light","Hướng hiển thị":"Display rotation","Hiệu ứng chuyển":"Page transition","Trượt ngang mượt":"Smooth horizontal slide","Trượt lên":"Slide up","Tắt hiệu ứng":"No animation","Thời gian chuyển":"Transition duration","Thời gian mỗi trang":"Time per page","giây":"seconds","Thời gian trang logo":"Logo page duration","Tự động chuyển trang":"Automatically rotate pages","Làm mượt chuyển động của số liệu và vòng đo":"Smooth value and gauge animation","Nhiệt độ":"Temperature","Quạt":"Cooling","Hiệu năng":"Performance","Logo khởi động":"Startup logo","Thứ tự hiển thị":"Display order","Sử dụng nút lên và xuống để sắp xếp các trang đã bật.":"Use the up and down buttons to reorder enabled pages.",
    "Bố cục tổng quan 240×240":"240×240 overview layout","Tùy chỉnh tiêu đề, góc bo và màu của từng thẻ thông tin.":"Customize the title, corner radius, and color of every information card.","Tiêu đề màn hình":"Display title","Độ bo góc":"Corner radius","Hiển thị ngày, tháng và năm":"Show date","Quạt CPU":"CPU fan","Quạt GPU":"GPU fan","Khôi phục màu mặc định":"Restore default colors",
    "Kiểu chữ và màu hiển thị":"Display typography and colors","Tùy chỉnh độ lớn, độ đậm và màu chữ cho màn hình 240×240.":"Adjust text size, weight, and color for the 240×240 display.","Cỡ chữ tổng thể":"Overall text size","Tiêu đề in đậm":"Bold headings","Nhãn thông số in đậm":"Bold metric labels","Giá trị in đậm":"Bold values","Sử dụng màu chữ tùy chỉnh":"Use custom text colors","Màu chữ chính":"Primary text color","Màu chữ phụ":"Secondary text color","Khôi phục kiểu chữ mặc định":"Restore default typography","Chọn hướng phù hợp với vị trí lắp đặt. Thay đổi được áp dụng ngay trên thiết bị.":"Select the rotation that matches the installation. Changes are applied immediately to the device.",
    "Đèn RGB trên bo mạch":"On-board RGB LED","Chọn màu, hiệu ứng và độ sáng của đèn trạng thái.":"Select the status LED color, effect, and brightness.","Hiệu ứng":"Effect","Màu tĩnh":"Solid color","Nhịp thở":"Breathing","Cầu vồng":"Rainbow","Theo nhiệt độ":"Temperature reactive","Theo mức tải":"Load reactive","Tốc độ hiệu ứng":"Effect speed",
    "Wi-Fi và đồng bộ thời gian":"Wi-Fi and time synchronization","Kết nối thiết bị với Internet để đồng bộ ngày giờ và kiểm tra bản cập nhật. Mạng cấu hình":"Connect the device to the Internet to synchronize the clock and check for updates. The setup network","vẫn được duy trì.":"remains available.","Quét mạng Wi-Fi":"Scan Wi-Fi networks","Chọn một mạng sau khi quét":"Select a network after scanning","Chưa quét":"Not scanned","Tên mạng Wi-Fi":"Wi-Fi network name","Mật khẩu Wi-Fi":"Wi-Fi password","Múi giờ":"Time zone","Việt Nam • UTC+7":"Vietnam • UTC+7","Xóa mật khẩu đã lưu (Wi‑Fi mở)":"Clear saved password (open Wi-Fi)","Lưu và kết nối":"Save and connect","Đồng bộ thời gian":"Time synchronization","Giờ":"Time","Ngày":"Date","Địa chỉ mạng":"Network address","Địa chỉ cấu hình":"Setup address","Cường độ sóng":"Signal strength",
    "Trung tâm cập nhật trực tuyến":"Online update center","Thiết bị tải thông tin phát hành công khai từ GitHub qua kết nối HTTPS và không lưu thông tin đăng nhập GitHub.":"The device downloads public release information from GitHub over HTTPS and never stores GitHub credentials.","Đang chờ kết nối Internet":"Waiting for Internet connection","Mã thiết bị":"Device ID","Địa chỉ MAC Wi-Fi":"Wi-Fi MAC address","Kênh phát hành":"Release channel","Trạng thái quản lý":"Management status","Thư viện hiệu ứng":"Effect library","Đồng bộ các hiệu ứng được phát hành cho PCScreen và lưu lựa chọn trên thiết bị.":"Synchronize published PCScreen effects and save the selection on the device.","0 hiệu ứng":"0 effects","Chưa tải danh sách hiệu ứng.":"The effect list has not been downloaded.","Cấu hình trang hiển thị":"Display page configuration","Đồng bộ ngôn ngữ, các trang được bật và thứ tự hiển thị.":"Synchronize language, enabled pages, and display order.","Áp dụng cấu hình":"Apply configuration","Phiên bản hiện tại":"Current version","Phiên bản mới nhất":"Latest version","Kiểm tra cập nhật":"Check for updates","Tải và cài đặt":"Download and install","Mở trang quản trị":"Open admin console","Thiết bị sẽ tự kiểm tra khi có kết nối Internet.":"The device checks automatically when an Internet connection is available.","Xác thực cập nhật:":"Update validation:","thiết bị kiểm tra loại bo mạch, dung lượng firmware và mã SHA‑256 trước khi cài đặt.":"the device verifies the board, firmware size, and SHA-256 digest before installation.",
    "Ảnh khởi động 240×240":"240×240 startup image","Cắt và căn chỉnh ảnh ngay trong trình duyệt. Ảnh GIF được giữ nguyên chuyển động và tự căn giữa trên màn hình. Dung lượng tối đa:":"Crop and align the image in the browser. Animated GIFs retain motion and are centered automatically. Maximum size:","Dung lượng đã sử dụng":"Storage used","Giới hạn mỗi tệp":"Per-file limit","Chưa chọn ảnh":"No image selected","Phóng to":"Zoom","Vị trí ngang":"Horizontal position","Vị trí dọc":"Vertical position","Căn giữa":"Center","Thời gian hiển thị":"Display duration","Xử lý và tải lên":"Process and upload","Xóa ảnh":"Delete image",
    "Thông tin và điều khiển thiết bị":"Device information and controls","Theo dõi tài nguyên hệ thống và thực hiện các thao tác quản trị cơ bản.":"Monitor system resources and perform basic administration.","Phiên bản firmware":"Firmware version","Thời gian hoạt động":"Uptime","Dung lượng flash":"Flash size","Trạng thái màn hình":"Display status","Khởi động lại thiết bị":"Restart device","Cập nhật firmware từ tệp":"Update firmware from a file","Chọn tệp":"Select a","dành cho PCScreen. Duy trì nguồn điện ổn định trong suốt quá trình cập nhật.":"file for PCScreen. Keep power stable throughout the update.","Tệp firmware":"Firmware file","Bắt đầu cập nhật":"Start update","Sẵn sàng cập nhật.":"Ready to update.","Đang chờ tệp firmware…":"Waiting for a firmware file…","Không ngắt nguồn khi quá trình cập nhật đang diễn ra.":"Do not disconnect power while the update is in progress.","Phát triển bởi Nguyễn Tiến Thành":"Developed by Nguyễn Tiến Thành"
  };
  Object.assign(en, {
    "Đã tạm dừng cập nhật":"Updates paused","Đang nhận dữ liệu":"Receiving data","Chưa nhận dữ liệu từ máy tính":"No computer data received","Windows Agent đã kết nối":"Windows Agent connected","Đang kết nối lại":"Reconnecting","Đã kết nối":"Connected","Đã tạm dừng cập nhật dữ liệu":"Telemetry updates paused","Đã áp dụng tần suất mới":"New update rate applied","Đã đọc dữ liệu hiện có trên thiết bị":"Current device data loaded","Đã sao chép":"Copied","Không thể sao chép":"Copy failed","Đang bật":"On","Đang tắt":"Off","Đã tắt":"Off","Bật":"On","Tắt":"Off","Đã lưu cài đặt":"Settings saved","Đã áp dụng giao diện MI50":"MI50 interface applied","Đang quét…":"Scanning…","Không tìm thấy mạng Wi-Fi":"No Wi-Fi networks found","Đã lưu Wi-Fi. Thiết bị đang kết nối lại.":"Wi-Fi saved. The device is reconnecting.","Đã đồng bộ thời gian":"Time synchronized","Chưa đồng bộ":"Not synchronized","Đã chọn":"Selected","Áp dụng":"Apply","Đã áp dụng hiệu ứng":"Effect applied","Đã áp dụng cấu hình trang":"Page configuration applied","Thiết bị đang khởi động lại…":"Device is restarting…","Lệnh đã được thực hiện":"Command completed","Đang kiểm tra bản cập nhật…":"Checking for updates…","Không có bản cập nhật mới.":"No new update is available.","Đang tải bản cập nhật…":"Downloading update…","Đang cài đặt firmware…":"Installing firmware…","Cập nhật thành công. Thiết bị sẽ khởi động lại.":"Update successful. The device will restart.","Đang xử lý ảnh…":"Processing image…","Đã tải ảnh khởi động":"Startup image uploaded","Đã xóa ảnh khởi động":"Startup image deleted","Không có ảnh khởi động":"No startup image","Đã căn giữa ảnh":"Image centered","Đã chọn tệp firmware":"Firmware file selected","Đang tải firmware…":"Uploading firmware…","Hoàn tất. Thiết bị đang khởi động lại…":"Complete. The device is restarting…","Đang khởi động lại thiết bị…":"Restarting device…",
    "Dữ liệu gửi đi không hợp lệ.":"The submitted data is invalid.","Độ sáng phải từ 0 đến 100%.":"Brightness must be between 0 and 100%.","Giá trị màu không hợp lệ.":"The color value is invalid.","Tốc độ hiệu ứng phải từ 1 đến 100%.":"Effect speed must be between 1 and 100%.","Hiệu ứng không được hỗ trợ.":"This effect is not supported.","Ngôn ngữ không được hỗ trợ.":"This language is not supported.","Hướng hiển thị không hợp lệ.":"The display rotation is invalid.","Không tìm thấy tài nguyên yêu cầu.":"The requested resource was not found.","Chỉ hỗ trợ ảnh PNG, JPG hoặc GIF.":"Only PNG, JPG, and GIF images are supported.","Tệp vượt quá giới hạn 1,2 MB.":"The file exceeds the 1.2 MB limit.","Vui lòng chọn đúng tệp .pcota.":"Select a valid .pcota file.","Tệp cập nhật không hợp lệ.":"The update file is invalid.","Firmware không dành cho ESP32-S3.":"This firmware is not for ESP32-S3.","Không thể cài đặt firmware.":"Firmware installation failed."
  });

  const dynamicPatterns = [
    [/^(\d+) ngày (\d+) giờ$/, "$1 days $2 hours"],
    [/^(\d+) giờ (\d+) phút$/, "$1 hours $2 minutes"],
    [/^(\d+) phút$/, "$1 minutes"],
    [/^(\d+(?:[.,]\d+)?) giây$/, "$1 seconds"],
    [/^(\d+)% tải$/, "$1% load"],
    [/^Đang bật • (.+)$/, "On • $1"],
    [/^Bật • (.+)$/, "On • $1"],
    [/^Dữ liệu đang được cập nhật mỗi (.+) ms\. Thiết bị đã sẵn sàng\.$/, "Data updates every $1 ms. The device is ready."],
    [/^Hướng hiển thị đã được đặt thành (.+)\.$/, "Display rotation set to $1."],
    [/^(\d+) hiệu ứng$/, "$1 effects"],
    [/^Đã tìm thấy (\d+) mạng$/, "Found $1 networks"]
  ];

  const originals = new WeakMap();
  let active = "vi", applying = false;
  function translatedText(value) {
    const compact = value.replace(/\s+/g, " ").trim();
    if (!compact) return value;
    let replacement = en[compact];
    if (!replacement) {
      const pattern = dynamicPatterns.find(item => item[0].test(compact));
      if (pattern) replacement = compact.replace(pattern[0], pattern[1]);
    }
    if (!replacement) return value;
    return value.match(/^\s*/)[0] + replacement + value.match(/\s*$/)[0];
  }
  function translateNode(node) {
    if (node.nodeType === Node.TEXT_NODE) {
      if (!originals.has(node)) originals.set(node, node.nodeValue);
      let source = originals.get(node);
      const expected = active === "en" ? translatedText(source) : source;
      if (node.nodeValue !== source && node.nodeValue !== expected) {
        source = node.nodeValue;
        originals.set(node, source);
      }
      const next = active === "en" ? translatedText(source) : source;
      if (node.nodeValue !== next) node.nodeValue = next;
      return;
    }
    if (node.nodeType !== Node.ELEMENT_NODE || node.tagName === "SCRIPT" || node.tagName === "STYLE") return;
    ["placeholder", "title", "aria-label", "alt"].forEach(attr => {
      if (!node.hasAttribute(attr)) return;
      const saved = originals.get(node) || {}, key = `attr:${attr}`;
      if (!(key in saved)) saved[key] = node.getAttribute(attr);
      originals.set(node, saved);
      node.setAttribute(attr, active === "en" ? translatedText(saved[key]) : saved[key]);
    });
    [...node.childNodes].forEach(translateNode);
  }
  function apply(language) {
    active = language === "en" ? "en" : "vi";
    document.documentElement.lang = active;
    applying = true; translateNode(document.body); applying = false;
  }
  new MutationObserver(records => {
    if (applying) return;
    applying = true;
    records.forEach(record => {
      if (record.type === "characterData") translateNode(record.target);
      record.addedNodes.forEach(translateNode);
    });
    applying = false;
  }).observe(document.documentElement, { subtree:true, childList:true, characterData:true });
  window.PCScreenI18n = { apply, t:(vi, english) => active === "en" ? english : vi };
})();
