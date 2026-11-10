# Minimal RAR5 extractor using the system libarchive via ctypes.
# usage: python3 -I rar_extract.py ARCHIVE OUTDIR
import ctypes, ctypes.util, os, sys
la = ctypes.CDLL(ctypes.util.find_library("archive") or "libarchive.so.13")
la.archive_read_new.restype = ctypes.c_void_p
la.archive_entry_pathname.restype = ctypes.c_char_p
la.archive_entry_pathname.argtypes = [ctypes.c_void_p]
la.archive_entry_filetype.restype = ctypes.c_uint
la.archive_entry_filetype.argtypes = [ctypes.c_void_p]
la.archive_error_string.restype = ctypes.c_char_p
la.archive_error_string.argtypes = [ctypes.c_void_p]
for f in ("archive_read_support_format_all", "archive_read_support_filter_all", "archive_read_free"):
    getattr(la, f).argtypes = [ctypes.c_void_p]
la.archive_read_open_filename.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_size_t]
la.archive_read_next_header.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p)]
la.archive_read_data.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
la.archive_read_data.restype = ctypes.c_ssize_t
AE_IFREG, AE_IFDIR = 0o100000, 0o040000
src, out = sys.argv[1], os.path.realpath(sys.argv[2])
a = la.archive_read_new(); la.archive_read_support_format_all(a); la.archive_read_support_filter_all(a)
if la.archive_read_open_filename(a, src.encode(), 1 << 16) != 0:
    sys.exit("open failed: %s" % la.archive_error_string(a))
entry = ctypes.c_void_p(); buf = ctypes.create_string_buffer(1 << 20)
n_ok = n_bad = 0
while True:
    r = la.archive_read_next_header(a, ctypes.byref(entry))
    if r == 1: break                      # ARCHIVE_EOF
    if r < -20: sys.exit("fatal: %s" % la.archive_error_string(a))
    name = la.archive_entry_pathname(entry).decode("utf-8", "replace").replace("\\", "/")
    dest = os.path.realpath(os.path.join(out, name))
    if os.path.isabs(name) or not dest.startswith(out + os.sep):
        print("SKIP unsafe path:", name); continue
    ft = la.archive_entry_filetype(entry)
    if ft == AE_IFDIR: os.makedirs(dest, exist_ok=True); continue
    if ft != AE_IFREG: print("SKIP non-regular:", name); continue
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    ok = True
    with open(dest, "wb") as fh:
        while True:
            k = la.archive_read_data(a, buf, len(buf))
            if k == 0: break
            if k < 0: ok = False; print("ERR", name, la.archive_error_string(a)); break
            fh.write(buf.raw[:k])
    n_ok += ok; n_bad += (not ok)
la.archive_read_free(a)
print("extracted %d files, %d errors" % (n_ok, n_bad))
