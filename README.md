# ETDTimer 🕒⚡

![ETDTimer Logo](etdtimer.png)

**ETDTimer**, Windows işletim sistemleri için C++ (Win32 API & GDI+) ile geliştirilmiş, ultra hafif, yüksek performanslı ve sürekli ekranda duran (Floating Always-On-Top) bir saat, kronometre, sayaç ve Pomodoro yardımcı uygulamasıdır.

---

## 🌟 Özellikler / Features

1. **Saat (Clock)**:
   - Varsayılan olarak Saat ve Dakika (SS:DK) gösterimi.
   - Ayarlar menüsünden istenirse saniye (SS:DK:SS) görünümü açılabilir.
2. **Kronometre (Stopwatch)**:
   - Başlat, Durdur, Sıfırla ve sınırsız Tur / Checkpoint ekleme özelliği.
3. **Sayaç (Timer)**:
   - **Süre Sayımı**: Belirli bir dakika/saat boyunca geriye sayım.
   - **Hedef Saate Sayım**: Gün içerisindeki belirli bir saate kadar (ör. 17:30) geriye sayım.
   - Süre dolduğunda özel sesli alarm bildirimi.
4. **Pomodoro Zamanlayıcı**:
   - Çalışma süresi, mola süresi, toplam hedef çalışma saati ve mola sayısı bilgilerine göre otomatik periyot planlaması yapar.
   - Çalışma ve mola periyotları arasında otomatik geçiş ve sesli bildirim sağlar.
5. **Tam Ekran Algılama (Auto-Hide on Fullscreen)**:
   - Oyun oynarken veya tam ekran video izlerken uygulamayı otomatik olarak gizler / alta alır; masaüstüne dönüldüğünde tekrar en üste gelir.
6. **Merdiven (Staircase / Accordion) Düzeni**:
   - Başlangıçta sadece üstteki Saat çubuğu görünür. Sol alttaki 3 çizgili burger menüden eklenecek araçlar alta basamak stili eklenir.
   - Tek bir araçtan en fazla 3 adet, toplamda en fazla 10 araç açılabilir. Ekran sığmadığında otomatik olarak yan yana ızgara düzenine geçer.
   - Üstteki göz ikonu ile tüm araçlar anında gizlenebilir.
7. **Çift Dil & Tema Desteği**:
   - Türkçe ve İngilizce dil seçeneği.
   - Açık (Light) ve Koyu (Dark) tema desteği.
   - Özel MP3/WAV alarm sesi seçebilme.

---

## 📦 Kurulum Sihirbazı / Custom Setup Wizard

Uygulama için 3. parti hiçbir yazılım kullanılmadan **C++ Win32 API** ile özel **`ETDTimerSetup.exe`** kurulum sihirbazı geliştirilmiştir.

- Kurulum başında Türkçe / İngilizce dil seçeneği sunar.
- Masaüstü Kısayolu ve Başlangıç kaydı (Autostart) seçeneği sunar.
- `C:\Users\<Kullanıcı>\AppData\Local\Programs\ETDTimer` dizinine bağımsız olarak kurulur.

---

## 🛠️ Derleme / Build Instructions

Projeyi Windows üzerinde MinGW-w64 (`g++.exe`) ile derlemek için:

```powershell
powershell -ExecutionPolicy Bypass -File build_and_sign.ps1
```

Bu işlem:
1. `res/resource.rc` dosyasını derler.
2. `ETDTimer.exe` ve `ETDTimerSetup.exe` binary dosyalarını C++20 standardında sıfır harici DLL bağımlılığıyla derler.
3. Dijital kod imzalama sertifikasını (Code Signing) otomatik oluşturup dijital olarak imzalar.

---

## ⚖️ Yasal Bildirim & Lisans / Legal Disclaimer & License

Proje yapımcısının, Emir Tuğra Dağ, uygulamadaki herhangi bir şeyi düzeltme, uygulamaya yeni özellik getirme veya güncelleme gibi bir sorumluluğu yok. Proje olduğu gibi sunulmakta ve olası iyi veya kötü hiç bir olayda geliştirici Emir Tuğra Dağ sorumlu olamaz. Kod tabanları MIT lisansına tabi olup projenin adı ve logolarının hakları Emir Tuğra Dağ'da saklıdır ve izinsiz kullanılamaz.

Distributed under the **MIT License**. Copyright (c) 2026 **Emir Tuğra Dağ**.
