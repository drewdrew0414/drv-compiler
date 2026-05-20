// dri installer launcher (WinForms GUI) — compile with csc.exe → dri-install.exe
// Build: build-installer.bat
using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Windows.Forms;

[assembly: AssemblyTitle("dri Installer")]
[assembly: AssemblyProduct("dri Language Compiler")]
[assembly: AssemblyCompany("drewdrew0414")]
[assembly: AssemblyCopyright("Copyright © drewdrew0414")]
[assembly: AssemblyDescription("dri HPC systems programming language installer")]
[assembly: AssemblyVersion("0.1.0.0")]
[assembly: AssemblyFileVersion("0.1.0.0")]

namespace DriInstaller
{
    static class Program
    {
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new SetupForm());
        }
    }

    // ── Main setup form ─────────────────────────────────────────────────────────
    class SetupForm : Form
    {
        private RichTextBox rtbEula;
        private CheckBox    chkAgree;
        private Button      btnInstall;
        private Button      btnCancel;
        private Label       lblTitle;
        private Label       lblSub;
        private Label       lblEulaHdr;
        private Panel       pnlTop;
        private Panel       pnlBottom;

        public SetupForm()
        {
            BuildUI();
        }

        void BuildUI()
        {
            // ── Form settings ───────────────────────────────────────────────────
            this.Text            = "dri 언어 컴파일러 v0.1.0 설치";
            this.Size            = new Size(620, 580);
            this.MinimumSize     = new Size(620, 580);
            this.StartPosition   = FormStartPosition.CenterScreen;
            this.FormBorderStyle = FormBorderStyle.FixedDialog;
            this.MaximizeBox     = false;
            this.BackColor       = Color.FromArgb(245, 247, 250);

            // ── Top header panel ────────────────────────────────────────────────
            pnlTop = new Panel();
            pnlTop.Dock      = DockStyle.Top;
            pnlTop.Height    = 88;
            pnlTop.BackColor = Color.FromArgb(30, 40, 60);

            lblTitle = new Label();
            lblTitle.Text      = "dri 언어 컴파일러";
            lblTitle.ForeColor = Color.White;
            lblTitle.Font      = new Font("Segoe UI", 18f, FontStyle.Bold);
            lblTitle.AutoSize  = true;
            lblTitle.Location  = new Point(20, 14);

            lblSub = new Label();
            lblSub.Text      = "HPC 시스템 프로그래밍 언어  v0.1.0  |  drvpm-registry.cloud";
            lblSub.ForeColor = Color.FromArgb(150, 175, 210);
            lblSub.Font      = new Font("Segoe UI", 9f);
            lblSub.AutoSize  = true;
            lblSub.Location  = new Point(22, 54);

            pnlTop.Controls.Add(lblTitle);
            pnlTop.Controls.Add(lblSub);

            // ── EULA header label ───────────────────────────────────────────────
            lblEulaHdr = new Label();
            lblEulaHdr.Text     = "다음 이용약관을 읽고 동의하셔야 설치를 진행할 수 있습니다:";
            lblEulaHdr.Font     = new Font("Segoe UI", 9f);
            lblEulaHdr.AutoSize = true;
            lblEulaHdr.Location = new Point(20, 102);

            // ── EULA body text ──────────────────────────────────────────────────
            rtbEula = new RichTextBox();
            rtbEula.Location    = new Point(20, 124);
            rtbEula.Size        = new Size(572, 308);
            rtbEula.ReadOnly    = true;
            rtbEula.BackColor   = Color.White;
            rtbEula.Font        = new Font("Segoe UI", 9f);
            rtbEula.BorderStyle = BorderStyle.FixedSingle;
            rtbEula.ScrollBars  = RichTextBoxScrollBars.Vertical;
            rtbEula.Text        = GetEulaText();

            // ── Agreement checkbox ──────────────────────────────────────────────
            chkAgree = new CheckBox();
            chkAgree.Text     = "위 이용약관을 모두 읽었으며, 이에 동의합니다.";
            chkAgree.Font     = new Font("Segoe UI", 9.5f);
            chkAgree.AutoSize = true;
            chkAgree.Location = new Point(20, 446);
            chkAgree.CheckedChanged += delegate { btnInstall.Enabled = chkAgree.Checked; };

            // ── Bottom button panel ─────────────────────────────────────────────
            pnlBottom = new Panel();
            pnlBottom.Dock      = DockStyle.Bottom;
            pnlBottom.Height    = 62;
            pnlBottom.BackColor = Color.FromArgb(232, 236, 242);

            btnInstall = new Button();
            btnInstall.Text      = "설치  ▶";
            btnInstall.Size      = new Size(112, 38);
            btnInstall.Location  = new Point(376, 12);
            btnInstall.Enabled   = false;
            btnInstall.FlatStyle = FlatStyle.Flat;
            btnInstall.BackColor = Color.FromArgb(0, 120, 215);
            btnInstall.ForeColor = Color.White;
            btnInstall.Font      = new Font("Segoe UI", 10f, FontStyle.Bold);
            btnInstall.FlatAppearance.BorderSize = 0;
            btnInstall.Click += OnInstallClicked;

            btnCancel = new Button();
            btnCancel.Text      = "취소";
            btnCancel.Size      = new Size(90, 38);
            btnCancel.Location  = new Point(502, 12);
            btnCancel.FlatStyle = FlatStyle.Flat;
            btnCancel.BackColor = Color.FromArgb(198, 204, 214);
            btnCancel.ForeColor = Color.FromArgb(30, 40, 60);
            btnCancel.Font      = new Font("Segoe UI", 10f);
            btnCancel.FlatAppearance.BorderSize = 0;
            btnCancel.Click += delegate { Application.Exit(); };

            pnlBottom.Controls.Add(btnInstall);
            pnlBottom.Controls.Add(btnCancel);

            // ── Add controls ────────────────────────────────────────────────────
            this.Controls.Add(pnlTop);
            this.Controls.Add(lblEulaHdr);
            this.Controls.Add(rtbEula);
            this.Controls.Add(chkAgree);
            this.Controls.Add(pnlBottom);
        }

        // ── Install button click ─────────────────────────────────────────────
        void OnInstallClicked(object sender, EventArgs e)
        {
            btnInstall.Enabled = false;
            btnCancel.Enabled  = false;
            btnInstall.Text    = "다운로드 중...";

            string ps1Url  = "https://drvpm-registry.cloud/dist/v0.1.0/window/install.ps1";
            string ps1Path = Path.Combine(Path.GetTempPath(), "dri-install-" + Guid.NewGuid().ToString("N").Substring(0, 8) + ".ps1");

            try
            {
                using (var wc = new System.Net.WebClient())
                {
                    wc.Headers.Add("User-Agent", "dri-installer/0.1.0");
                    wc.DownloadFile(ps1Url, ps1Path);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(
                    "install.ps1 다운로드에 실패했습니다.\n\n" +
                    "URL: " + ps1Url + "\n\n" +
                    "오류: " + ex.Message + "\n\n" +
                    "인터넷 연결을 확인하세요.",
                    "다운로드 오류",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error
                );
                btnInstall.Enabled = true;
                btnCancel.Enabled  = true;
                btnInstall.Text    = "설치  ▶";
                return;
            }

            btnInstall.Text = "설치 중...";
            RunInstaller(ps1Path, tryElevate: true);
        }

        void RunInstaller(string ps1Path, bool tryElevate)
        {
            try
            {
                var psi = new ProcessStartInfo();
                psi.FileName        = "powershell.exe";
                psi.Arguments       = "-NoProfile -ExecutionPolicy Bypass -File \"" + ps1Path + "\"";
                psi.UseShellExecute = true;
                if (tryElevate) psi.Verb = "runas";

                this.Hide();
                var proc = Process.Start(psi);
                if (proc != null) proc.WaitForExit();
                UpdateSystemEnvironment();

                try { File.Delete(ps1Path); } catch { }

                MessageBox.Show(
                    "dri 컴파일러 설치 및 PATH 자동 등록이 완료되었습니다.\n\n" +
                    "새 터미널(cmd, PowerShell 등)을 열고 다음 명령어로 확인하세요:\n" +
                    "  dri --version",
                    "설치 완료",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information
                );
                Application.Exit();
            }
            catch
            {
                if (tryElevate)
                {
                    RunInstaller(ps1Path, tryElevate: false);
                }
                else
                {
                    this.Show();
                    MessageBox.Show(
                        "PowerShell 실행에 실패했습니다.\n수동 설치를 진행하십시오.",
                        "설치 오류",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Error
                    );
                    btnInstall.Enabled = true;
                    btnCancel.Enabled  = true;
                    btnInstall.Text    = "설치  ▶";
                }
            }
        }

        [System.Runtime.InteropServices.DllImport("user32.dll", SetLastError = true, CharSet = System.Runtime.InteropServices.CharSet.Auto)]
        private static extern IntPtr SendMessageTimeout(IntPtr hWnd, uint Msg, UIntPtr wParam, string lParam, uint fuFlags, uint uTimeout, out UIntPtr lpdwResult);

        void UpdateSystemEnvironment()
        {
            try
            {
                UIntPtr result;
                // HWND_BROADCAST (0xFFFF), WM_SETTINGCHANGE (0x001A)
                SendMessageTimeout((IntPtr)0xFFFF, 0x001A, UIntPtr.Zero, "Environment", 2, 1000, out result);
            }
            catch { }
        }

        // ── EULA text ───────────────────────────────────────────────────────────
        static string GetEulaText()
        {
            return
"dri 언어 컴파일러 최종 사용자 라이선스 계약 (EULA)\r\n" +
"버전 0.1.0\r\n" +
"━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\r\n" +
"\r\n" +
"본 계약은 귀하(개인 또는 법인)와 dri 언어 프로젝트 제작자(이하 \"제공자\") 사이의\r\n" +
"법적 계약입니다. 소프트웨어를 설치, 복사 또는 사용함으로써 본 계약 조건에\r\n" +
"동의한 것으로 간주됩니다. 동의하지 않으시면 설치를 중단하십시오.\r\n" +
"\r\n" +
"제1조 (라이선스 부여)\r\n" +
"제공자는 귀하에게 dri 컴파일러(이하 \"소프트웨어\")를 개인적, 교육적,\r\n" +
"상업적 목적으로 사용할 수 있는 비독점적, 양도 불가능한 라이선스를 부여합니다.\r\n" +
"\r\n" +
"제2조 (허용 행위)\r\n" +
"- 소프트웨어를 사용하여 dri 소스 파일(.dri)을 컴파일할 수 있습니다.\r\n" +
"- 팀 또는 조직 내에서 소프트웨어를 배포할 수 있습니다.\r\n" +
"- dri 언어로 작성한 프로그램의 저작권은 귀하에게 귀속됩니다.\r\n" +
"\r\n" +
"제3조 (금지 행위)\r\n" +
"- 소프트웨어를 역공학, 디컴파일 또는 분해할 수 없습니다.\r\n" +
"- 소프트웨어를 재판매하거나 독립 제품으로 재배포할 수 없습니다.\r\n" +
"- 소프트웨어의 저작권 표시를 제거하거나 변경할 수 없습니다.\r\n" +
"\r\n" +
"제4조 (패키지 레지스트리)\r\n" +
"본 소프트웨어는 drvpm-registry.cloud 레지스트리 서비스를 통해 컴파일러\r\n" +
"바이너리 및 패키지를 다운로드합니다. 레지스트리 이용 시 해당 서비스의\r\n" +
"별도 이용약관이 적용됩니다. 다운로드되는 패키지 콘텐츠에 대한 책임은\r\n" +
"해당 패키지 제작자에게 있습니다.\r\n" +
"\r\n" +
"제5조 (개인정보 처리)\r\n" +
"설치 과정에서 운영체제 버전 및 설치 결과 정보가 레지스트리 서버로 전송될\r\n" +
"수 있습니다. 수집된 정보는 서비스 개선 목적으로만 사용됩니다.\r\n" +
"\r\n" +
"제6조 (보증 부인)\r\n" +
"소프트웨어는 \"있는 그대로\" 제공됩니다. 제공자는 상품성, 특정 목적 적합성,\r\n" +
"비침해성에 대해 명시적 또는 묵시적 보증을 하지 않습니다.\r\n" +
"\r\n" +
"제7조 (책임 제한)\r\n" +
"어떠한 경우에도 제공자는 본 소프트웨어 사용 또는 사용 불능으로 인한\r\n" +
"직접적, 간접적, 우발적, 결과적 손해에 대해 책임을 지지 않습니다.\r\n" +
"\r\n" +
"제8조 (오픈소스 구성요소)\r\n" +
"본 소프트웨어는 MIT, Apache 2.0 등 오픈소스 라이선스 하의 구성요소를\r\n" +
"포함할 수 있습니다. 각 구성요소 라이선스는 설치 디렉토리의 LICENSES\r\n" +
"파일에서 확인할 수 있습니다.\r\n" +
"\r\n" +
"제9조 (계약 종료)\r\n" +
"본 계약 조건을 위반하면 라이선스가 자동 종료됩니다. 종료 시 소프트웨어의\r\n" +
"모든 복사본을 삭제해야 합니다.\r\n" +
"\r\n" +
"제10조 (준거법)\r\n" +
"본 계약은 대한민국 법률에 따라 해석되고 집행됩니다.\r\n" +
"\r\n" +
"━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\r\n" +
"문의: drew0414drew@gmail.com\r\n" +
"프로젝트: https://github.com/drewdrew0414/dri-lang\r\n" +
"레지스트리: https://drvpm-registry.cloud\r\n";
        }
    }
}
