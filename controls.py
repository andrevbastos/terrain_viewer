import tkinter as tk
import socket
import random
import os

# Endereço de destino UDP
UDP_IP = "127.0.0.1"
UDP_PORT = 12345

class TerrainControlsApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Controle de Terrenos")
        self.root.geometry("420x840")
        self.root.configure(bg="#0b0b0c")
        self.root.resizable(False, False)
        
        # Socket UDP
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        
        # Variáveis de Controle
        self.var_width = tk.IntVar(value=256)
        self.var_wave = tk.DoubleVar(value=100.0)
        self.var_freq = tk.DoubleVar(value=4.0)
        self.var_amp = tk.DoubleVar(value=1.0)
        self.var_exp = tk.DoubleVar(value=1.0)
        self.var_seed = tk.IntVar(value=42)
        self.var_octaves = tk.IntVar(value=3)
        self.var_intensity = tk.DoubleVar(value=100.0)
        
        # Caminho da imagem temporária do preview
        self.preview_path = "build/noise_preview.png"
        self.zoomed_photo = None
        self.preview_mtime = None
        
        self.setup_ui()
        self.send_parameters()
        
        # Inicia a escuta/leitura periódica da imagem de preview gerada pelo C++
        self.poll_preview()
        
    def setup_ui(self):
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        
        # Painel central de controles
        self.center_frame = tk.Frame(self.root, bg="#0b0b0c")
        self.center_frame.grid(row=0, column=0, padx=20, pady=20, sticky="")
        
        # Título
        title_label = tk.Label(
            self.center_frame, 
            text="G E R A D O R  D E   T E R R E N O S", 
            font=("Helvetica", 10, "bold"), 
            bg="#0b0b0c", 
            fg="#ffffff"
        )
        title_label.pack(pady=(0, 20))
        
        # Sliders
        self.create_slider(self.center_frame, "Resolução do Grid", self.var_width, 64, 512, is_int=True)
        self.create_slider(self.center_frame, "Comprimento de Onda", self.var_wave, 10, 500)
        self.create_slider(self.center_frame, "Frequência Base", self.var_freq, 0.1, 10.0)
        self.create_slider(self.center_frame, "Amplitude Base", self.var_amp, 0.1, 2.0)
        self.create_slider(self.center_frame, "Expoente (Relevo)", self.var_exp, 0.5, 4.0)
        self.create_slider(self.center_frame, "Oitavas", self.var_octaves, 1, 8, is_int=True)
        self.create_slider(self.center_frame, "Intensidade Vertical", self.var_intensity, 10, 300)
        
        # Separador 1
        separator = tk.Frame(self.center_frame, height=1, bg="#1a1a1e")
        separator.pack(fill=tk.X, pady=15)
        
        # Seed e Reseed
        seed_frame = tk.Frame(self.center_frame, bg="#0b0b0c")
        seed_frame.pack(fill=tk.X)
        
        seed_label = tk.Label(
            seed_frame, 
            text="SEED:", 
            font=("Helvetica", 8, "bold"), 
            bg="#0b0b0c", 
            fg="#808085"
        )
        seed_label.pack(side=tk.LEFT)
        
        self.seed_entry = tk.Entry(
            seed_frame, 
            textvariable=self.var_seed, 
            width=10, 
            font=("Helvetica", 9), 
            bg="#16161a", 
            fg="#ffffff", 
            insertbackground="#ffffff",
            relief=tk.FLAT,
            bd=4
        )
        self.seed_entry.pack(side=tk.LEFT, padx=10)
        self.seed_entry.bind("<KeyRelease>", lambda e: self.send_parameters())
        
        reseed_btn = tk.Button(
            seed_frame, 
            text="RESEED", 
            command=self.reseed, 
            bg="#ffffff", 
            fg="#0b0b0c", 
            activebackground="#e0e0e6", 
            activeforeground="#0b0b0c",
            font=("Helvetica", 8, "bold"),
            relief=tk.FLAT,
            bd=0,
            padx=14,
            pady=5,
            cursor="hand2"
        )
        reseed_btn.pack(side=tk.RIGHT)
        
        # Separador 2
        separator2 = tk.Frame(self.center_frame, height=1, bg="#1a1a1e")
        separator2.pack(fill=tk.X, pady=15)
        
        # Label do Título do Preview
        preview_title = tk.Label(
            self.center_frame, 
            text="PREVIEW DO RUÍDO", 
            font=("Helvetica", 7, "bold"), 
            bg="#0b0b0c", 
            fg="#808085"
        )
        preview_title.pack(pady=(0, 5))
        
        # Container de Imagem do Preview com borda fina
        preview_border = tk.Frame(self.center_frame, bg="#1a1a1e", padx=1, pady=1)
        preview_border.pack(pady=5)
        
        self.preview_label = tk.Label(
            preview_border, 
            text="Aguardando render...",
            font=("Helvetica", 8),
            bg="#0b0b0c", 
            fg="#808085",
            width=24,
            height=12
        )
        self.preview_label.pack()
        
    def create_slider(self, parent, label_text, var, from_val, to_val, is_int=False):
        frame = tk.Frame(parent, bg="#0b0b0c")
        frame.pack(fill=tk.X, pady=6)
        
        label_frame = tk.Frame(frame, bg="#0b0b0c")
        label_frame.pack(fill=tk.X)
        
        title = tk.Label(
            label_frame, 
            text=label_text.upper(), 
            font=("Helvetica", 7, "bold"), 
            bg="#0b0b0c", 
            fg="#808085"
        )
        title.pack(side=tk.LEFT)
        
        val_label = tk.Label(
            label_frame, 
            text="", 
            font=("Helvetica", 8, "bold"), 
            bg="#0b0b0c", 
            fg="#ffffff"
        )
        val_label.pack(side=tk.RIGHT)
        
        def update_val(val):
            formatted_val = f"{int(float(val))}" if is_int else f"{float(val):.2f}"
            val_label.config(text=formatted_val)
            self.send_parameters()
            
        slider = tk.Scale(
            frame, 
            from_=from_val, 
            to=to_val, 
            orient=tk.HORIZONTAL, 
            variable=var, 
            resolution=1 if is_int else 0.05,
            showvalue=False,
            bg="#16161a",
            fg="#ffffff",
            highlightthickness=0,
            troughcolor="#16161a",
            activebackground="#ffffff",
            command=update_val,
            relief=tk.FLAT,
            bd=0,
            length=350,
            sliderlength=14
        )
        slider.pack(fill=tk.X, pady=(3, 0))
        
        initial_val = var.get()
        val_label.config(text=f"{initial_val}" if is_int else f"{initial_val:.2f}")

    def reseed(self):
        new_seed = random.randint(0, 999999)
        self.var_seed.set(new_seed)
        self.send_parameters()

    def poll_preview(self):
        # Tenta carregar a imagem gerada pelo C++ do disco de forma não-bloqueante
        if os.path.exists(self.preview_path):
            try:
                mtime = os.path.getmtime(self.preview_path)
                if mtime != self.preview_mtime:
                    # Carrega o PNG salvo pelo C++ somente quando ele muda
                    photo = tk.PhotoImage(file=self.preview_path)
                    
                    # Atualiza a label para exibir a imagem no tamanho real fixo
                    self.preview_label.config(image=photo, text="", width=0, height=0)
                    self.zoomed_photo = photo # Mantém a referência na memória
                    self.preview_mtime = mtime
            except Exception:
                pass
                
        # Agenda a próxima verificação para dali 80ms
        self.root.after(80, self.poll_preview)

    def send_parameters(self):
        try:
            seed_val = self.var_seed.get()
        except tk.TclError:
            return

        w = self.var_width.get()
        h = w
        wave = self.var_wave.get()
        freq = self.var_freq.get()
        amp = self.var_amp.get()
        exp = self.var_exp.get()
        seed = seed_val
        octaves = self.var_octaves.get()
        intensity = self.var_intensity.get()
        
        # Envia parâmetros ao C++
        msg = f"{w} {h} {wave:.4f} {freq:.4f} {amp:.4f} {exp:.4f} {seed} {octaves} {intensity:.4f}"
        try:
            self.sock.sendto(msg.encode(), (UDP_IP, UDP_PORT))
        except Exception:
            pass

if __name__ == "__main__":
    root = tk.Tk()
    app = TerrainControlsApp(root)
    root.mainloop()
