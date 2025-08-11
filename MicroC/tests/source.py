import pathlib
import subprocess
import unittest

print("Source tests for microC using microCJIT")

MICROCJIT = "./build/microCJIT"
TEST_DIR = pathlib.Path(__file__).parent


def run_source(source: str, arg: int = 0):
    path = TEST_DIR.joinpath(source)
    result = subprocess.run([MICROCJIT, path, str(arg)], capture_output=True)

    if result.stderr:
        print(f"\nError: {result.stderr.decode()}")

    return result


class One(unittest.TestCase):
    def test_10(self):
        result = run_source("ex/ex1.c", 10)
        stdout = result.stdout.strip()

        self.assertEqual(stdout, b"10 9 8 7 6 5 4 3 2 1")

    def test_0(self):
        result = run_source("ex/ex1.c", 0)
        stdout = result.stdout.strip()

        self.assertEqual(stdout, b"")


class Two(unittest.TestCase):
    def test_10(self):
        result = run_source("ex/ex2.c")
        stdout = result.stdout.strip()

        details = stdout.split(b" ")

        # i != p
        self.assertNotEqual(details[0], details[1])

        # p == &i, internal
        self.assertEqual(details[2], b"1")

        # p == &i, external
        self.assertEqual(details[1], details[4])

        # *p = i = 227
        self.assertEqual(details[5], b"227")

        # *&i = i = 12
        self.assertEqual(details[6], b"12")

        # *p = i = 12
        self.assertEqual(details[7], b"12")

        # ia[0] = 14
        self.assertEqual(details[8], b"14")

        # ia[9] = 114
        self.assertEqual(details[9], b"114")

        # TODO: FIXME
        # ipa[2] = p = &i
        # self.assertEqual(details[10], details[1])

        # TODO: FIXME
        # *ipa[2] = *p = i
        # self.assertEqual(details[10], b"12")

        # &*ipa[2] == &**(ipa + 2), internal
        self.assertEqual(details[12], b"1")

        # &(*iap)[2] == &*((*iap) + 2), internal
        self.assertEqual(details[13], b"1")


class Three(unittest.TestCase):
    def test_10(self):
        result = run_source("ex/ex3.c", 10)
        stdout = result.stdout.strip()

        self.assertEqual(stdout, b"0 1 2 3 4 5 6 7 8 9")


class Four(unittest.TestCase):
    def test_10(self):
        result = run_source("ex/ex4.c", 10)
        stdout = result.stdout.strip()

        self.assertEqual(stdout, b"1 1 2 6 24 120 720 5040 40320 362880")


class Five(unittest.TestCase):
    # Square fn, by pointer
    def test_0(self):
        result = run_source("ex/ex5.c", 0)
        stdout = result.stdout.strip()

        self.assertEqual(stdout, b"0 0")

    def test_10(self):
        result = run_source("ex/ex5.c", 10)
        stdout = result.stdout.strip()

        self.assertEqual(stdout, b"100 10")


class Six(unittest.TestCase):
    def test_5(self):
        result = run_source("ex/ex6.c", 5)
        stdout = result.stdout.strip()

        # FIXME: Scope issues
        self.assertEqual(stdout, b"1 1 2 6 24 5")


class Ten(unittest.TestCase):
    def test_5(self):
        result = run_source("ex/ex10.c", 5)
        stdout = result.stdout.strip()

        self.assertEqual(stdout, b"1 1 2 6 24 5")


class Eleven(unittest.TestCase):
    def test_5(self):
        result = run_source("ex/ex11.c", 11)
        stdout = result.stdout.strip()

        solutions = stdout.split(b"\n")

        self.assertEqual(len(solutions), 2680)
        self.assertEqual(solutions[0], b"1 3 5 7 9 11 2 4 6 8 10 ")


class Twelve(unittest.TestCase):
    def test_5(self):
        result = run_source("ex/ex12.c", 5)

        self.assertEqual(result.returncode, 17)


class Thirteen(unittest.TestCase):
    def test_5(self):
        result = run_source("ex/ex13.c", 1920)
        stdout = result.stdout.strip()

        self.assertEqual(stdout, b"1892 1896 1904 1908 1912 1916 1920")


class Fourteen(unittest.TestCase):
    def test_1(self):
        result = run_source("ex/ex14.c", 1)
        stdout = result.stdout.strip()

        self.assertEqual(stdout, b"1")

    def test_4(self):
        result = run_source("ex/ex14.c", 4)
        stdout = result.stdout.strip()

        self.assertEqual(stdout, b"2")

    def test_10(self):
        result = run_source("ex/ex14.c", 10)
        stdout = result.stdout.strip()

        self.assertEqual(stdout, b"4")


class Fifteen(unittest.TestCase):
    def test_2(self):
        result = run_source("ex/ex15.c", 2)
        stdout = result.stdout.strip()

        self.assertEqual(stdout, b"2 1 999999")


class Sixteen(unittest.TestCase):
    def test_0(self):
        result = run_source("ex/ex16.c", 0)
        stdout = result.stdout.strip()

        self.assertEqual(stdout, b"1111 2222")

    def test_1(self):
        result = run_source("ex/ex16.c", 1)
        stdout = result.stdout.strip()

        self.assertEqual(stdout, b"2222")


class Seventeen(unittest.TestCase):
    def test_2(self):
        result = run_source("ex/ex15.c", 2)
        stdout = result.stdout.strip()

        self.assertEqual(stdout, b"2 1 999999")


# FIXME: Requires support for multiple arguments
# class Eighteen(unittest.TestCase):
#     def test_0_1(self):
#         # result = run_source("ex/ex15.c", 2)
#         # stdout = result.stdout.strip()

#         self.assertTrue(False)


class Nineteen(unittest.TestCase):
    def test_0(self):
        result = run_source("ex/ex19.c", 0)
        stdout = result.stdout.strip()

        self.assertEqual(stdout, b"33")

    def test_1(self):
        result = run_source("ex/ex19.c", 1)
        stdout = result.stdout.strip()

        self.assertEqual(stdout, b"44")


# FIXME: Requires support for multiple arguments
# class Twenty(unittest.TestCase):
#     def test_0_1(self):
#         # result = run_source("ex/ex15.c", 2)
#         # stdout = result.stdout.strip()

#         self.assertTrue(False)


class TwentyOne(unittest.TestCase):
    def test_2(self):
        result = run_source("ex/ex21.c", 2)
        stdout = result.stdout.strip()
        lines = stdout.split(b"\n")

        self.assertEqual(lines[0], b"0 7 7 117 ")
        self.assertEqual(lines[1], b"1 7 7 117")


# FIXME: Fn returns
class TwentyTwo(unittest.TestCase):
    def test_5(self):
        result = run_source("ex/ex22.c", 1920)
        stdout = result.stdout.strip()

        self.assertEqual(stdout, b"1892 1896 1904 1908 1912 1916 1920")


class TwentyThree(unittest.TestCase):
    def test_10(self):
        result = run_source("ex/ex23.c", 10)
        stdout = result.stdout
        lines = stdout.split(b"\n")

        self.assertEqual(lines[0], b"0 1 ")
        self.assertEqual(lines[1], b"1 1 ")
        self.assertEqual(lines[2], b"2 2 ")
        self.assertEqual(lines[3], b"3 3 ")
        self.assertEqual(lines[4], b"4 5 ")
        self.assertEqual(lines[5], b"5 8 ")
        self.assertEqual(lines[6], b"6 13 ")
        self.assertEqual(lines[7], b"7 21 ")
        self.assertEqual(lines[8], b"8 34 ")
        self.assertEqual(lines[9], b"9 55 ")


class TwentyFour(unittest.TestCase):
    def test_2(self):
        result = run_source("ex/ex24.c")
        stdout = result.stdout.strip()

        self.assertEqual(stdout, b"125 1021")


if __name__ == "__main__":
    _ = unittest.main()
