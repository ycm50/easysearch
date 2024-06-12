import os  
import tkinter as tk  
from tkinter import filedialog, messagebox  
  
class FileSearcherApp:  
    def __init__(self, root):  
        self.root = root  
        self.root.title("文件查询器")  
  
        # 输入框  
        self.entry_char = tk.Entry(self.root)  
        self.entry_char.pack(pady=5)  
  
        # 列表框  
        self.listbox_files = tk.Listbox(self.root, height=10, width=50)  
        self.listbox_files.pack(pady=10)  
  
        # 按钮  
        tk.Label(self.root, text="请输入查询的字符:").pack(pady=10)  
        search_button = tk.Button(self.root, text="查询文件", command=self.search_files_with_char)  
        search_button.pack(pady=5)  
  
        open_button = tk.Button(self.root, text="打开选中的文件", command=self.open_selected_file)  
        open_button.pack(pady=5)  
  
    def search_files_with_char(self):  
        char = self.entry_char.get()  
        if not char:  
            messagebox.showerror("错误", "请输入查询的字符")  
            return  
  
        cur_dir = os.getcwd()  
        names = os.listdir(cur_dir)  
        tar_names = [(name, os.path.join(cur_dir, name)) for name in names if char in name]  
  
        # 清除之前的列表  
        self.listbox_files.delete(0, tk.END)  
  
        # 填充列表  
        for idx, (name, path) in enumerate(tar_names, 1):  
            self.listbox_files.insert(tk.END, f"{idx}、{name}")  
  
    def open_selected_file(self):  
        selected_index = self.listbox_files.curselection()  
        if not selected_index:  
            messagebox.showerror("错误", "请选择要打开的文件")  
            return  
    
        index = selected_index[0]  # 假设只选择一个文件  
        # 假设Listbox中的每个条目都是一个形如 "1、filename.txt" 的字符串  
        selected_item = self.listbox_files.get(index)  
        # 解析条目以获取文件名（这里只是一个简单的示例，你可能需要更复杂的解析）  
        parts = selected_item.split('、')  # 假设文件名前有一个“编号、”的前缀  
        if len(parts) > 1:  
            filename = parts[1]  # 获取文件名部分  
            # 构建文件路径（这里假设文件在当前工作目录下）  
            path_to_open = os.path.join(os.getcwd(), filename)  
    
            # 检查文件是否存在  
            if os.path.exists(path_to_open):  
                os.startfile(path_to_open)  
                messagebox.showinfo("成功", f"已打开文件: {path_to_open}")  
            else:  
                messagebox.showerror("错误", f"文件不存在: {path_to_open}")  
        else:  
            messagebox.showerror("错误", "无法解析文件名")
  
def main():  
    root = tk.Tk()  
    app = FileSearcherApp(root)  
    root.mainloop()  
  
if __name__ == "__main__":  
    main()